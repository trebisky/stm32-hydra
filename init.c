/* init.c
 * (c) Tom Trebisky  11-24-2020
 *
 * The First C code that runs right after startup.
 *
 *  Zero BSS area.
 *  ....
 */

#include "hydra.h"

/* These are arranged in the LDS script */
/* We don't need all these, but they were
 * interesting when sorting all this out and
 * do no harm (no extra memory is used.
 */
extern char __bss_start;
extern char __bss_end;
extern char __end;

extern int  __data_start;
extern int  __data_end;

extern int  __text_start;
extern int  __text_end;

extern unsigned int __rodata_start;
extern unsigned int __rodata_end;

static void
setup_default_serial ( void )
{
		int fd;

#ifdef CHIP_F405
        fd = serial_begin ( UART2, 115200 );
#else
        fd = serial_begin ( UART1, 115200 );
#endif
        set_std_serial ( fd );
}

/* Perform various magic before the user code gets started.
 */
void
stm_init ( void )
{
	int *p;
	int *src;

	/* Zero the BSS area */
	for ( p = (int *) &__bss_start; p < (int *) &__bss_end; )
	    *p++ = 0;

	/* Copy initialized data from flash */
	// src = &__rodata_start;
	src = &__text_end;

	for ( p = &__data_start; p < &__data_end; p++ )
		*p = *src++;

	ram_init ();
	rcc_init ();

	setup_default_serial ();
	printf ( "Rebooted -- initializing\n" );

	systick_init ();
	nvic_init ();

	usb_init ();

	/* So we can use scope on clocks */
    gpio_mco_setup ();

	led_init ();
	led_off ();

	/* Not needed, the chip starts up with
	 * interrupts enabled
	 */
	// enable_irq;

	/* Call user code */
	startup ();
}

#ifdef notdef

/* This stuff is here from the BSS demo */

/* This is a good a place as any for some notes.
 * What goes on here is tied intimately to the LDS file.
 * Before I started doing the .data segment properly,
 * here is what was going on.  I didn't mention .data
 * at all, and the linker followed some default and put
 * it into flash.  This works, but the variables are
 * immutable, just as if they were .rodata.
 *
 * Then I added a .data segment to the LDS file and went
 * crazy for a few hours because that tried to put it into
 * RAM, and it gets random crap (naturally).
 *
 * When I told it to put .data into flash, I was working
 * again, but with immutable variables.  It was time to
 * add the loop above.
 *
 * So we have 3 kinds of data:
 *
 * 1) .bss - in ram, gets initialized to 0
 * 2) .data - in ram, gets set to values from flash
 * 3) .rodata - in flash, immutable values.
 */

static int demo_1 = 999;
static const int demo_2 = 123;

/* When I run what is below, I see:
 *  BSS start: 20000000
 *  BSS end: 20000048
 *  Data start: 20000048
 *  Data end: 20000068
 *  End: 20000068
 *  Text start: 08000000
 *  Text end: 080012BC
 *  RO Data start: 080012DC
 *  RO Data end: 080014CC
 *  Demo 1: 20000048  <-- .data in ram
 *  Demo 2: 080012DC  <-- .rodata in flash
 */

void
init_show ( void )
{
	printf ( "BSS start: %X\n", &__bss_start );
	printf ( "BSS end: %X\n", &__bss_end );

	printf ( "Data start: %X\n", &__data_start );
	printf ( "Data end: %X\n", &__data_end );

	/* Heap could start here */
	printf ( "End: %X\n", &__end );

	printf ( "Text start: %X\n", &__text_start );
	printf ( "Text end: %X\n", &__text_end );

	printf ( "RO Data start: %X\n", &__rodata_start );
	printf ( "RO Data end: %X\n", &__rodata_end );

	printf ( "Demo 1: %X\n", &demo_1 );
	printf ( "Demo 2: %X\n", &demo_2 );
}
#endif

/* =========================================================== */

/* These are default entrys in the interrupt vector
 * If we don't spin, we end up in a furious interrupt loop
 * since we don't cancel the interrupt
 * No telling if we will get the messages or not,
 *  but we usually do.
 */
void
bogus ( void )
{
	printf ( "Unexpected interrupt!\n" );
	printf ( "Spinning\n" );
	for ( ;; ) ;
}

void
fault ( void )
{
	printf ( "Unexpected fault!\n" );
	printf ( "Spinning\n" );
	for ( ;; ) ;
}

/* THE END */
