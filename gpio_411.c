/* gpio.c
 * (c) Tom Trebisky  9-24-2016
 * (c) Tom Trebisky  11-20-2020
 *
 * Basic GPIO driver for the F411
 *
 * Also includes LED routines
 */

#include "hydra.h"

/* Here is the F411 gpio structure.
 *  very different from the F103.
 */
struct gpio {
	volatile unsigned int mode;	/* 0x00 */
	volatile unsigned int otype;	/* 0x04 */
	volatile unsigned int ospeed;	/* 0x08 */
	volatile unsigned int pupd;	/* 0x0c */
	volatile unsigned int idata;	/* 0x10 */
	volatile unsigned int odata;	/* 0x14 */
	volatile unsigned int bsrr;	/* 0x18 */
	volatile unsigned int lock;	/* 0x1c */
	volatile unsigned int afl;	/* 0x20 */
	volatile unsigned int afh;	/* 0x24 */
};

/* On the F411, we have 3 gpio
 *  (more in the chip, but not routed on our 48 pin package).
 * We really only have A and B available.
 * PC 13 is the onboard LED
 * PC 14 and 15 are routed to the board edge, but are connected to
 * the 32 kHz crystal, so I can't see how that would be useful.
 * But PA and PB give us plenty of pins.
 * --
 * On the F429, we have a package with more pins,
 *  we have gpio A to G available.
 */

#define GPIOA_BASE	(struct gpio *) 0x40020000
#define GPIOB_BASE	(struct gpio *) 0x40020400
#define GPIOC_BASE	(struct gpio *) 0x40020800

#define GPIOD_BASE	(struct gpio *) 0x40020c00
#define GPIOE_BASE	(struct gpio *) 0x40021000
#define GPIOF_BASE	(struct gpio *) 0x40021400
#define GPIOG_BASE	(struct gpio *) 0x40021800

#define GPIOH_BASE	(struct gpio *) 0x40021c00
#define GPIOI_BASE	(struct gpio *) 0x40022000
#define GPIOJ_BASE	(struct gpio *) 0x40022400
#define GPIOK_BASE	(struct gpio *) 0x40022800

static struct gpio *gpio_bases[] = {
    GPIOA_BASE, GPIOB_BASE, GPIOC_BASE,
    GPIOD_BASE, GPIOE_BASE, GPIOF_BASE, GPIOG_BASE,
    GPIOH_BASE, GPIOI_BASE, GPIOJ_BASE, GPIOK_BASE
};

/* ================================================ */

/* Change alternate function setting for a pin
 * These are 4 bit fields. All initially 0.
 */
static void
gpio_af ( int gpio, int pin, int val )
{
	struct gpio *gp;
	int shift;

	gp = gpio_bases[gpio];

	if ( pin < 8 ) {
	    shift = pin * 4;
	    gp->afl &= ~(0xf<<shift);
	    gp->afl |= val<<shift;
	} else {
	    shift = (pin-8) * 4;
	    gp->afh &= ~(0xf<<shift);
	    gp->afh |= val<<shift;
	}
}

#define MODE_INPUT	0	/* input (reset state) */
#define MODE_OUT	1	/* output */
#define MODE_AF		2	/* alternate fn */
#define MODE_ANALOG	3

/* This is a 2 bit field */
static void
gpio_mode ( int gpio, int pin, int val )
{
	struct gpio *gp;
	int shift;

	gp = gpio_bases[gpio];

	shift = pin * 2;
	gp->mode &= ~(0x3<<shift);
	gp->mode |= val<<shift;
}

#define TYPE_PP		0	/* push pull (reset state) */
#define TYPE_OD		1	/* open drain */

/* This is a 1 bit field */
static void
gpio_otype ( int gpio, int pin, int val )
{
	struct gpio *gp;

	gp = gpio_bases[gpio];
	gp->pupd &= ~(0x1<<pin);
	gp->pupd |= val<<pin;
}

#define PUPD_NONE	0
#define PUPD_UP		1
#define PUPD_DOWN	2
#define PUPD_XXX	3

/* This is a 2 bit field */
static void
gpio_pupd ( int gpio, int pin, int val )
{
	struct gpio *gp;
	int shift;

	gp = gpio_bases[gpio];
	shift = pin * 2;
	gp->pupd &= ~(0x3<<shift);
	gp->pupd |= val<<shift;
}

#define SPEED_LOW	0
#define SPEED_MED	1
#define SPEED_FAST	2
#define SPEED_HIGH	3

/* This is a 2 bit field */
static void
gpio_ospeed ( int gpio, int pin, int val )
{
	struct gpio *gp;
	int shift;

	gp = gpio_bases[gpio];
	shift = pin * 2;
	gp->ospeed &= ~(0x3<<shift);
	gp->ospeed |= val<<shift;
}

/* ========================================= */

/* Configure as output pin, open drain */
void
gpio_output_od_config ( int gpio, int pin )
{
	gpio_mode ( gpio, pin, MODE_OUT );
	gpio_otype ( gpio, pin, TYPE_OD );
	gpio_ospeed ( gpio, pin, SPEED_HIGH );
}

/* Configure as output pin, push pull */
void
gpio_output_pp_config ( int gpio, int pin )
{
	gpio_mode ( gpio, pin, MODE_OUT );
	gpio_otype ( gpio, pin, TYPE_PP );
	gpio_ospeed ( gpio, pin, SPEED_HIGH );
}

/* Configure as input pin with pull up */
void
gpio_input_config ( int gpio, int pin )
{
	gpio_mode ( gpio, pin, MODE_INPUT );
	gpio_pupd ( gpio, pin, PUPD_UP );
}

/* Read an input pin */
int
gpio_read ( int gpio, int pin )
{
	struct gpio *gp = gpio_bases[gpio];

	return (gp->idata & (1<<pin)) >> pin;
}

/* Write an output pin */
void
gpio_bit ( int gpio, int pin, int val )
{
	struct gpio *gp = gpio_bases[gpio];

	if ( val )
	    gp->bsrr = 1 << (pin+16);
	else
	    gp->bsrr = 1 << pin;
}

/* kludge for now */
void
gpio_uart ( int gpio, int pin )
{
	// struct gpio *gp;
	// int shift;

	// gp = gpio_bases[gpio];

	gpio_otype ( gpio, pin, TYPE_PP );
	// gp->otype &= ~(1<<pin);

	// shift = pin * 2;
	// gp->ospeed |= (3<<shift);
	gpio_ospeed ( gpio, pin, SPEED_HIGH );

	// gp->pupd &= ~(3<<shift);
	gpio_pupd ( gpio, pin, PUPD_NONE );
}

static void
gpio_uart_pin_setup ( int gpio, int pin )
{
	gpio_af ( gpio, pin, 7 );
	gpio_mode ( gpio, pin, MODE_AF );
	gpio_uart ( gpio, pin );
}

/* Note that UART1 can be moved around a lot.
 * I make a choice here.
 * I suppose a general interface would allow this to
 *  be selected via a call argument.
 */
void
gpio_uart_init ( int uart )
{
	if ( uart == UART1 ) {
#ifdef CHIP_F407
	    gpio_uart_pin_setup ( GPIOB, 6 );	/* Tx */
	    gpio_uart_pin_setup ( GPIOB, 7 );	/* Rx */
#else
	    gpio_uart_pin_setup ( GPIOA, 9 );	/* Tx */
	    gpio_uart_pin_setup ( GPIOA, 10 );	/* Rx */
#endif
	    // gpio_af ( GPIOA, 9, 7 );
	    // gpio_mode ( GPIOA, 9, MODE_AF );
	    // gpio_uart ( GPIOA, 9 );

	    // gpio_af ( GPIOA, 10, 7 );
	    // gpio_mode ( GPIOA, 10, MODE_AF );
	    // gpio_uart ( GPIOA, 10 );
#ifdef notdef
	    gpio_uart_pin_setup ( GPIOA, 9 );	/* Tx */
	    gpio_uart_pin_setup ( GPIOA, 10 );	/* Rx */
#endif

	    /* UART1 could be on any of these pins
	     * if we wanted to move it there.
	     */
	    // gpio_af ( GPIOA, 15, 7 ); /* Tx */
	    // gpio_af ( GPIOB, 3, 7 );	/* Rx */

	    // gpio_af ( GPIOB, 6, 7 )	/* Tx */
	    // gpio_af ( GPIOB, 7, 7 );	/* Rx */
	} else if ( uart == UART2 ) {
	    gpio_af ( GPIOA, 2, 7 );	/* Tx */
	    gpio_mode ( GPIOA, 2, MODE_AF );
	    gpio_uart ( GPIOA, 2 );
	    gpio_af ( GPIOA, 3, 7 );	/* Rx */
	    gpio_mode ( GPIOA, 3, MODE_AF );
	    gpio_uart ( GPIOA, 3 );
	} else { /* UART3 */
	    gpio_af ( GPIOC, 6, 7 );	/* Tx */
	    gpio_mode ( GPIOC, 6, MODE_AF );
	    gpio_uart ( GPIOC, 6 );
	    gpio_af ( GPIOC, 7, 7 );	/* Rx */
	    gpio_mode ( GPIOC, 7, MODE_AF );
	    gpio_uart ( GPIOC, 7 );
	}
}

void
gpio_mco_pin_setup ( int gpio, int pin )
{
	    gpio_af ( gpio, pin, 0 );	/* 0 = System */
	    gpio_mode ( gpio, pin, MODE_OUT );
	    gpio_ospeed ( gpio, pin, SPEED_HIGH );
	    gpio_otype ( gpio, pin, TYPE_PP );
	    gpio_pupd ( gpio, pin, PUPD_NONE );
}

void
gpio_mco_setup ( void )
{
		gpio_mco_pin_setup ( GPIOA, 8 );
		gpio_mco_pin_setup ( GPIOC, 9 );
}

void
gpio_led_pin_setup ( int gpio, int pin )
{
	    gpio_af ( gpio, pin, 0 );	/* 0 = System */
	    gpio_mode ( gpio, pin, MODE_OUT );
	    gpio_ospeed ( gpio, pin, SPEED_HIGH );
	    gpio_otype ( gpio, pin, TYPE_OD );
	    gpio_pupd ( gpio, pin, PUPD_NONE );
}

void
gpio_usb_pin_setup ( int gpio, int pin, int alt )
{
	    gpio_af ( gpio, pin, alt );	/* 10 or 12 */
	    gpio_mode ( gpio, pin, MODE_AF );
	    gpio_ospeed ( gpio, pin, SPEED_HIGH );
	    gpio_otype ( gpio, pin, TYPE_PP );
	    gpio_pupd ( gpio, pin, PUPD_NONE );
}

/* THE END */
