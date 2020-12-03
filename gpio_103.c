/* gpio_103.c
 * (c) Tom Trebisky  7-2-2017, 9-12-2017
 * (c) Tom Trebisky  12-2-2020
 *
 * This is documented in section 9 of the reference manual.
 *
 * Started with lcd/gpio.c
 */

#include "hydra.h"

/* One of the 3 gpios */
struct gpio {
	volatile unsigned int cr[2];	/* 0x00 */
	volatile unsigned int idr;	/* 0x08 - input data (16 bits) */
	volatile unsigned int odr;	/* 0x0C - output data (16 bits) */
	volatile unsigned int bsrr;	/* 0x10 - set/reset register */
	volatile unsigned int brr;	/* 0x14 - reset register */
	volatile unsigned int lock;	/* 0x18 - lock registers (17 bits) */
};

/* We can reset a bit in two places, either via the brr,
 * or the top 16 bits of the bsrr.
 */

#define GPIOA_BASE	((struct gpio *) 0x40010800)
#define GPIOB_BASE	((struct gpio *) 0x40010C00)
#define GPIOC_BASE	((struct gpio *) 0x40011000)

static struct gpio *gpio_bases[] = {
    GPIOA_BASE, GPIOB_BASE, GPIOC_BASE
};

/* ================================================================ */

/* Each gpio has 16 bits.
 * The CR registers each control 8 of these bits.
 * cr[0] == cr_low  configures 0-7
 * cr[1] == cr_high configures 8-15
 * Each bit is controlled by a 4 bit field
 * The reset state is 0x4 for all (input, floating)
 */

/* there are only 3 choices for inputs */
#define INPUT_ANALOG    0x0
#define INPUT_FLOAT     0x4
#define INPUT_PUPD      0x8

/* For outputs, combine one from the following list of 3
 * with one of the 4 that follow.
 */
#define OUTPUT_10M      1
#define OUTPUT_2M       2
#define OUTPUT_50M      3

#define OUTPUT_PUSH_PULL        0
#define OUTPUT_ODRAIN           4

#define ALT_PUSH_PULL           8
#define ALT_ODRAIN              0xc

/* ================================================================ */

#ifdef notdef
#define MODE_IN		0x00	/* Input */
#define MODE_OUT_10	0x01	/* Output, 10 Mhz */
#define MODE_OUT_2	0x02	/* Output, 2 Mhz */
#define MODE_OUT_50	0x03	/* Output, 50 Mhz */

/* Output configurations */
#define CONF_GP_PP	0x0	/* GPIO - Pull up/down */
#define CONF_GP_OD	0x4	/* GPIO - Open drain */
#define CONF_ALT_PP	0x8	/* Alternate function - Pull up/down */
#define CONF_ALT_OD	0xC	/* Alternate function - Open drain */

/* Input configurations */
#define CONF_IN_ANALOG	0x0	/* Analog input */
#define CONF_IN_FLOAT	0x4	/* normal (as reset) */
#define CONF_IN_PP	0x8	/* with push/pull */

//#define OUT_CONF	(MODE_OUT_50 | CONF_GP_OD)
#define OUT_CONF	(MODE_OUT_50 | CONF_GP_PP)
#endif

#ifdef notdef
static void
bit_output ( struct gpio *gp, int bit )
{
	int conf;
	int shift;

	if ( bit < 8 ) {
	    shift = bit * 4;
	    conf = gp->crl & ~(0xf<<shift);
	    gp->crl = conf | OUT_CONF << shift;
	} else {
	    shift = (bit - 8) * 4;
	    conf = gp->crh & ~(0xf<<shift);
	    gp->crh = conf | OUT_CONF << shift;
	}
}

void
gpio_output ( int gpio, int pin )
{
        struct gpio *gp = gpio_bases[gpio];
	int conf;
	int shift;

	if ( pin < 8 ) {
	    shift = pin * 4;
	    conf = gp->crl & ~(0xf<<shift);
	    gp->crl = conf | OUT_CONF << shift;
	} else {
	    shift = (pin - 8) * 4;
	    conf = gp->crh & ~(0xf<<shift);
	    gp->crh = conf | OUT_CONF << shift;
	}
}

#endif

static void
gpio_mode ( int gpio, int bit, int mode )
{
        int reg;
        int conf;
        int shift;
        struct gpio *gp = gpio_bases[gpio];

        reg = bit / 8;
        shift = (bit%8) * 4;

        conf = gp->cr[reg] & ~(0xf<<shift);
        gp->cr[reg] = conf | (mode << shift);
}

void
gpio_output ( int gpio, int pin )
{
	/* For LED */
	gpio_mode ( gpio, pin, OUTPUT_2M | OUTPUT_ODRAIN );
	// gpio_mode ( gpio, pin, OUT_CONF );

	/* Another way */
	// gpio_mode ( gpio, pin, OUTPUT_50M | OUTPUT_PUSH_PULL );
}

/* Write a value to an output pin */
void
gpio_bit ( int gpio, int pin, int val )
{
        struct gpio *gp = gpio_bases[gpio];

        if ( val )
            gp->bsrr = 1 << (pin+16);
        else
            gp->bsrr = 1 << pin;
}

void
gpio_uart_init ( int uart )
{
	if ( uart == UART1 ) {
	    gpio_mode ( GPIOA, 9, OUTPUT_50M | ALT_PUSH_PULL );
	    // gpio_mode ( GPIOA, 10, INPUT_FLOAT );
	} else if ( uart == UART2 ) {
	    gpio_mode ( GPIOA, 2, OUTPUT_50M | ALT_PUSH_PULL );
	    // gpio_mode ( GPIOA, 3, INPUT_FLOAT );
	} else { /* UART3 */
	    gpio_mode ( GPIOB, 10, OUTPUT_50M | ALT_PUSH_PULL );
	    // gpio_mode ( GPIOB, 11, INPUT_FLOAT );
	}
}

/* Untested */
void
gpio_input_init ( int gpio, int pin )
{
	/* We would like it input, with pullup */
	gpio_mode ( gpio, pin, INPUT_PUPD );
}

/* Untested */
int
gpio_read ( int gpio, int pin )
{
        struct gpio *gp = gpio_bases[gpio];

        return gp->idr & (1<<pin);
}

/* THE END */
