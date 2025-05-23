/* event.c
 * Tom Trebisky 12-1-2020
 *
 * Code to handle "event" and "repeat"
 *
 * Taken from Kyu.
 */

#include "hydra.h"

/* ========================================================= */

/* This really belongs elsewhere.
 * This is the simplest possible memory allocator,
 * with no facility to free memory already allocated.
 * We maintain 4 byte alignment.
 */

extern char __end;
extern char __bss_end;
extern char __data_end;

static unsigned int ram_next;

void *
ram_alloc ( int size )
{
	void *rv;

	rv = (void *) ram_next;
	ram_next += size;
	ram_next &= 0xfffffffc;

	return rv;
}

void
ram_init ( void )
{
	// struct event *ep;

	// printf ( "__end = %X\n", &__end );
	// printf ( "__bss_end = %X\n", &__bss_end );
	// printf ( "__data_end = %X\n", &__data_end );

#ifdef notdef
	/* I see:
	    END: 20000060
	    SP: 2001FFD8
	*/
	printf ( "END: %X\n", &__end );
	printf ( "SP: %X\n", get_sp() );
#endif

	ram_next = (unsigned int) &__end;
	ram_next &= 0xfffffffc;

#ifdef notdef
	printf ( "ram next: %X\n", ram_next );
	printf ( "alloc: %d bytes\n", sizeof(struct event) );
	ep = (struct event *) ram_alloc ( sizeof(struct event) );
	printf ( "ep: %X\n", ep );
	printf ( "ram end: %X\n", ram_end );
#endif
}


/* ========================================================= */

struct event {
	struct event *next;
	int delay;
	int rep_count;
	int rep_reload;
	int id;
	vfptr func;
};

/* Statistics */
static int num_repeats = 0;
static int num_events = 0;
static int num_free = 0;

void
show_events ( void )
{
	printf ( "Num active repeats = %d\n", num_repeats );
	printf ( "Num active events = %d\n", num_events );
	printf ( "Num free events = %d\n", num_free );
}

static volatile int delay_counts = 0;

/* Holds lists of events and repeats waiting
 * on timer events.
 */
static struct event *event_freelist = 0;

static struct event *event_head = 0;
static struct event *repeat_head = 0;
static unsigned int event_id = 1;

static struct event *
event_alloc ( void )
{
        struct event *ep;

	if ( event_freelist ) {
	    ep = event_freelist;
	    event_freelist = ep->next;
	    --num_free;
	    // printf ( "Ealloc 1: %X\n", ep );
	} else {
	    ep = (struct event *) ram_alloc ( sizeof(struct event) );
	    // printf ( "Ealloc 2: %X\n", ep );
	}
	ep->id = event_id++;
}

static void
event_free ( struct event *ep )
{
    ep->next = event_freelist;
    event_freelist = ep;
    ++num_free;
}

/* Expects to be called with
 * interrupts locked.
 * (or by interrupt code).
 */
static void
remove_event ( struct event *ep )
{
        struct event *lp;

        if ( event_head == ep ) {
            event_head = ep->next;
            if ( event_head )
                event_head->delay += ep->delay;
        } else {
            for ( lp = event_head; lp; lp = lp->next )
                if ( lp == ep ) {
                    lp = ep->next;
                    if ( lp )
                        lp->delay += ep->delay;
                }
        }

	event_free ( ep );
	--num_events;
}

/* Called once for every timer interrupt.
 * (i.e. from systick at 1000 Hz)
 * *** Runs at interrupt level.
 * Handles events and repeats.
 */
void
event_tick ( void )
{
        struct event *ep;

	// printf ( "%d\n", delay_counts );

	/* Handle "delay" */
	if ( delay_counts )
	    --delay_counts;

        /* Process events */
        if ( event_head ) {
            --event_head->delay;
            while ( event_head && event_head->delay == 0 ) {
                ep = event_head;
                event_head = event_head->next;
		(*ep->func) ();
		remove_event ( ep );
            }
        }

        /* Process repeats.
         *  repeats are not like events where we only have to fool
         *  with the head of the list, we loop through all of them
         *  and keep them in no particular order.
         */
        for ( ep=repeat_head; ep; ep = ep->next ) {
            --ep->rep_count;
            if ( ep->rep_count < 1 ) {
                ep->rep_count = ep->rep_reload;
		(*ep->func) ();
            }
        }
}

/* maintain a linked list of folks waiting on
 * timer event (delay) activations.
 * In time-honored fashion, the list is kept in
 * sorted order, with the soon to be scheduled
 * entries at the front.  Each tick then just
 * needs to decrement the leading entry, and
 * when it becomes zero, one or more entries
 * get launched.
 */
static void
setup_event ( struct event *ep, int delay )
{
        struct event *p, *lp;

        p = event_head;

        while ( p && p->delay <= delay ) {
            delay -= p->delay;
            lp = p;
            p = p->next;
        }

        if ( p )
            p->delay -= delay;

        ep->delay = delay;
        ep->next = p;

        if ( p == event_head )
            event_head = ep;
        else
            lp->next = ep;
}

/* Public */
int
event ( int delay, vfptr fn )
{
	struct event *ep;

	ep = event_alloc ();
	ep->func = fn;
	++num_events;

	irq_disable ();
	setup_event ( ep, delay );
	irq_enable ();

	return ep->id;
}

/* Public */
int
repeat ( int delay, vfptr fn )
{
	struct event *ep;

	ep = event_alloc ();
	num_repeats++;

	ep->func = fn;

        ep->rep_reload = delay;
        ep->rep_count = delay;

        /* add to front of list */
	irq_disable ();
        ep->next = repeat_head;
        repeat_head = ep;
	irq_enable ();

	return ep->id;
}

/* Rarely called, if ever.
 * There is less of an issue with a race since
 * interrupt code never removes or cancels the repeat.
 * XXX - note if this gets called from interrupt level
 * we wrongly disable/enable IRQ.
 */
void
repeat_cancel ( int id )
{
        struct event *ep;
        struct event *cp;
        struct event *pp;

        for ( ep=repeat_head; ep; ep = ep->next ) {
	    if ( ep->id == id )
		break;
	}

	if ( ! ep )
	    return;

	irq_disable ();

        if ( repeat_head == ep ) {
            repeat_head = ep->next;
        } else {
	    pp = repeat_head;
	    cp = pp->next;
	    while ( cp ) {
		if ( cp == ep )
		    pp->next = cp->next;
		else
		    pp = cp;
		cp = pp->next;
	    }
	}

	irq_enable ();

	event_free ( ep );
	--num_repeats;
}

/* This gets called from thr_unblock() when we
 * unblock a thread with a running delay timer.
 * could be called from interrupt code.
 * We lock interrupts to avoid a race with the
 * interrupt code, which could also decide to
 * remove the event.
 */
void
event_cancel ( int id )
{
        struct event *ep;

	irq_disable ();

        for ( ep=event_head; ep; ep = ep->next ) {
	    if ( ep->id == id )
		break;
	}

	if ( ep )
	    remove_event ( ep );

	irq_enable ();
}

/* ======================================================================= */

/* These are sort of related to the above in that they
 * involve synchronization between interrupt code and
 * non-interrupt code.
 */

/* The idea here is that this is an "idle loop", i.e. a place
 * for the processor to sit and wait for interrupts.
 * This was originally just a hard spin loop.
 * Now I use "wfe" - there is also "wfi" and people talk
 * about masking interrupts before launching one of these,
 * which sounds like exactly the wrong thing to do,
 * but if you let the processor come out of wfe and then
 * immediately unmask interrupts, maybe that is the thing to
 * do and will let the processor enter a deeper sleep mode.
 * It certainly does work.
 */
void
idle ( void )
{
	for ( ;; ) {
	    /*
	    irq_disable ();
	    asm volatile( "wfe" );
	    irq_enable ();
	    */
	    asm volatile( "wfi" );
	}
}

/* This is the guts of the above that you can wrap in your
 * own loop.  Note that it will wake up on EVERY interrupt,
 * and given that systick is running at 1000 Hz, you will get
 * at least that.
 */
void
sleep ( void )
{
	/*
	irq_disable ();
	asm volatile( "wfe" );
	irq_enable ();
	*/
	asm volatile( "wfi" );
}

/* Using idle() is far better, yet this has its uses in
 * a few rare cases.
 */
void
spin ( void )
{
	for ( ;; ) ;
}

static void
loop_delay ( int d )
{
	volatile int x;

	for ( x=0; x<d; x++ )
	    ;
}

/* For some reason I have yet to sort out, my sysclk test
 * only works right -- when I have the interrupt sandwich
 * around wfe above -- when I add this delay.
 *  tjt  12-2-2020
 */
static void
dilly_dally ( void )
{
	// printf ( "Delay wake %d\n", delay_counts );
	// printf ( "Kilroy was here\n" );

	// Works
	// printf ( "XXXX\n" );

	// Works for a few seconds 
	// printf ( "XXX\n" );

	// Won't work
	// printf ( "XX\n" );

	// Works
	// loop_delay ( 50000 );
	// loop_delay ( 20000 );
#ifdef notdef
	/* we get one tick during the delay */
	printf ( "-%d\n", delay_counts );
	loop_delay ( 10000 );
	printf ( "-%d\n", delay_counts );
#endif
	loop_delay ( 10000 );

	// Nope
	// loop_delay ( 5000 );
}

/* User code can call this to sleep until a certain number
 * of systicks elapse, i.e a delay for so many milliseconds.
 */
void
delay ( int counts )
{
	irq_disable ();
	delay_counts = counts;
	irq_enable ();

	// printf ( "Delay enters sleep loop\n" );
	for ( ;; ) {
	    sleep ();
	    // dilly_dally ();
	    if ( ! delay_counts )
			break;
	}
}

static volatile char block_flag = 0;

/* These really have nothing to do with systick, but are here
 * (temporarily) because they are in essence a generalization
 * of the delay scheme above.  We have user code and a variety
 * of interrupt callback code.  This is a facility for user code
 * to wait until signaled by interrupt code.
 */
void
block ( void )
{
	irq_disable ();
	block_flag = 1;
	irq_enable ();

	for ( ;; ) {
	    sleep ();
	    if ( ! block_flag )
		break;
	}
}

void
unblock ( void )
{
	block_flag = 0;
}

/* =================================================================== */

/* delay_us() is sort of an orphan, but it needs to live someplace.
 * We use it to time really short delays using an evil delay loop.
 * The iic.c bit/bang i2c driver uses this to clock pulses.
 */

/* This is a scope loop to allow the delay constants to
 * be calibrated.  I have done this for a 96 Mhz STM32F411.
 */
#define DELAY_CAL_GPIO	GPIOA
#define DELAY_CAL_PIN	2

void delay_us ( int );
void delay_400k ( void );

void
delay_calibrate ( void )
{
	int pin = DELAY_CAL_PIN;

	/* Gotta use push pull to see anything on
	 * the scope.
	 */
	gpio_output_pp_config ( GPIOA, pin );

	for ( ;; ) {
	    // delay_us ( 5 );
	    delay_400k ();
	    gpio_bit ( GPIOA, pin, 0 );
	    // delay_us ( 5 );
	    delay_400k ();
	    gpio_bit ( GPIOA, pin, 1 );
	}
}

/* The following numbers were worked up using an oscilloscope and
 * a basic simple scope loop with delay_us(5).
 * This is what would be used by my bit/bang iic driver for i2c
 * which needs to generated 100 kHz.
 */

/* For 72 Mhz cpu - not verified */
#ifdef CHIP_F103
// #define DELAY_US_MULT	120	// 89 kHz
// #define DELAY_US_MULT	110	// 95.4 kHz
#define DELAY_US_MULT	105	// 100 kHz
// #define DELAY_US_MULT	100	// 105 kHz
#define DELAY_US_400K	11
#endif

/* For 96 Mhz F411 cpu - verified 12-2020 */
#if defined(CHIP_F411) && !defined(CHIP_F429)
// #define DELAY_US_MULT	300	// 101 kHz
#define DELAY_US_MULT	302	// 100 kHz

// #define DELAY_US_400K	38	// 1.4 us
#define DELAY_US_400K	32	// 403 kHz
#endif

/* For 168 Mhz F429 -- 3-12-2025
 * Simply scaled from the F411
 */
#ifdef CHIP_F429
#define DELAY_US_MULT	528
#define DELAY_US_400K	56
#endif

/* XXX - when we get the F429 running at 168 Mhz or something
 * other than 96, we will need to add another entry here,
 * or perhaps ask the RCC module what the actual CPU clock
 * is and adjust the delay accordingly.
 */

/* From libmaple.
 * Having the core loop in assembly makes this stay the
 * same regardless of compiler optimization levels or
 * changes in compiler code generation.
 */
void
delay_us ( int us )
{
    us *= DELAY_US_MULT;
    us /= 10;

    /* fudge for function call overhead  */
    us--;

    asm volatile("   mov r0, %[us]          \n\t"
                 "1: subs r0, #1            \n\t"
                 "   bhi 1b                 \n\t"
                 :
                 : [us] "r" (us)
                 : "r0");
}

/* I checked this for the F429 (while running at 96 Mhz)
 * using a stopwatch and it was just right.
 */
void
delay_ms ( int ms )
{
	while ( ms-- )
		delay_us ( 1000 );
}

/* This generates the delay for i2c at 400K, which would
 * be a 1.25 us delay
 */
void
delay_400k ( void )
{
    int count = DELAY_US_400K;

    asm volatile("   mov r0, %[count]          \n\t"
                 "1: subs r0, #1            \n\t"
                 "   bhi 1b                 \n\t"
                 :
                 : [count] "r" (count)
                 : "r0");
}

/* THE END */
