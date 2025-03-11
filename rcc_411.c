/* rcc.c
 * (c) Tom Trebisky  11-20-2020
 *
 * Reset and Clock Control for the STM32F411
 */

#include "hydra.h"

struct rcc {
	volatile unsigned int cr;	/* 0 - control reg */
	volatile unsigned int pll;	/* 4 - pll config */
	volatile unsigned int conf;	/* 8 - clock config */
	volatile unsigned int cir;	/* c - clock interrupt */
	volatile unsigned int ahb1_r;	/* 10 - AHB1 peripheral reset */
	volatile unsigned int ahb2_r;	/* 14 - AHB2 peripheral reset */
	int __pad1[2];
	volatile unsigned int apb1_r;	/* 20 - APB1 peripheral reset */
	volatile unsigned int apb2_r;	/* 24 - APB2 peripheral reset */
	int __pad2[2];
	volatile unsigned int ahb1_e;	/* 30 - AHB1 peripheral enable */
	volatile unsigned int ahb2_e;	/* 34 - AHB2 peripheral enable */
	int __pad3[2];
	volatile unsigned int apb1_e;	/* 40 - APB1 peripheral enable */
	volatile unsigned int apb2_e;	/* 44 - APB2 peripheral enable */
	int __pad4[2];
	volatile unsigned int ahb1_elp;	/* 50 - AHB1 peripheral enable in low power */
	volatile unsigned int ahb2_elp;	/* 54 - AHB2 peripheral enable in low power */
	int __pad5[2];
	volatile unsigned int apb1_elp;	/* 60 - APB1 peripheral enable in low power */
	volatile unsigned int apb2_elp;	/* 64 - APB2 peripheral enable in low power */
	int __pad6[2];
	volatile unsigned int bdcr;	/* 70 */
	volatile unsigned int csr;	/* 74 */
	int __pad7[2];
	volatile unsigned int sscgr;	/* 80 */
	volatile unsigned int plli2s;	/* 84 */
	int __pad8;
	volatile unsigned int dccr;	/* 8c */
};

#define RCC_BASE	(struct rcc *) 0x40023800

/* In the CR */
#define CR_HSEON	0x010000
#define CR_HSERDY	0x020000
#define CR_HSEBYP	0x040000

#define CR_PLLON	0x01000000
#define CR_PLLRDY	0x02000000

/* In the CONF */
#define CONF_CLOCK_BITS		0x3

/* CONF register ------------------------------------------------ */
#define CONF_HSI		0x0
#define CONF_HSE		0x1
#define CONF_PLL		0x2

#define APB1_SHIFT	10
#define APB2_SHIFT	13

#define APB_DIV1	0
#define APB_DIV2	4
#define APB_DIV4	5
#define APB_DIV8	6
#define APB_DIV16	7

/* Mux control for MCO1 and MCO2 */
#define MCO2_SYSCLK		0
#define MCO2_PLLI2S		0x40000000
#define MCO2_HSE		0x80000000
#define MCO2_PLL		0xC0000000

#define MCO1_HSI		0
#define MCO1_LSE		0x00020000
#define MCO1_HSE		0x00040000
#define MCO1_PLL		0x00060000

/* Prescaler values for MCO */
#define MCO_PRE_1		0
#define MCO_PRE_2		4
#define MCO_PRE_3		5
#define MCO_PRE_4		6
#define MCO_PRE_5		7

#define MCO1_PRE_SHIFT	24
#define MCO2_PRE_SHIFT	27

/* Peripheral enables -------------------------------------------- */
/* On AHB1 */
#define GPIOA_ENABLE	0x01
#define GPIOB_ENABLE	0x02
#define GPIOC_ENABLE	0x04
#define GPIOD_ENABLE	0x08
#define GPIOE_ENABLE	0x10
#define GPIOH_ENABLE	0x80

#define USB_HS_ENABLE	0x20000000

/* On AHB2 */
#define USB_ENABLE	0x80

/* On APB1 */
#define UART2_ENABLE	0x20000

/* On APB2 */
#define UART1_ENABLE	0x10
#define UART3_ENABLE	0x20
#define SYSCFG_ENABLE	BIT(14)

/* In the PLL register */

#define PLL_RESERVED	0xF83C8000

#define PLL_SRC_HSE	0x400000
#define PLL_SRC_HSI	0x0

#define PLL_N_SHIFT	6
#define PLL_P_SHIFT	16
#define PLL_Q_SHIFT	24

/* Only 2 bits for this with special values
 * actually these are (P/2)-1
 */
#define PLL_P_2		0
#define PLL_P_4		1
#define PLL_P_6		2
#define PLL_P_8		3

#define PLL_P_DIV_2	(PLL_P_2<<PLL_P_SHIFT)
#define PLL_P_DIV_4	(PLL_P_4<<PLL_P_SHIFT)
#define PLL_P_DIV_6	(PLL_P_6<<PLL_P_SHIFT)
#define PLL_P_DIV_8	(PLL_P_8<<PLL_P_SHIFT)

/* ===================================================================================== */
/* ===================================================================================== */
/* This code was originally developed and tested for the F411 based "black pill".
 * See section 6.2 in the TRM (RM0383)
 * The black pill has a 25 Mhz crystal.
 * The HSI is an internal 16 Mhz clock source.
 * The chip comes up initially running on this.
 * The HSE is the "high speed external" source (i.e. the 25 Mhz crystal).
 * We start this running, then configure the PLL to generate SYSCLK
 * from the HSE source. The PLL could use either HSI or HSE.
 * Once the PLL is running, we switch SYSCLK from HSI to the PLL.
 * SYSCLK is not yet the CPU clock (FCLK free running clock)
 * We have the option to divide SYSCLK to get FCLK.
 * The 48 Mhz clock for USB (and whatever else) also and only gets
 * generated by the PLL, but separately from SYSCLK.
 * There is a separate PLL for i2s clocks, but I so far ignore it.
 * ---
 * Here is how I set up the PLL for the F411 --
 * Let n = 192, m = 25 so VCO = HSE * n/m = 25*192/25 = 192 Mhz
 * Let p = 2 so that SYSCLK is vco/p = 192/2 = 96 Mhz
 * let q = 4 so that USB (OTG FS) = 192/4 = 48 Mhz
 * 
 * Note that the CPU could run at 100, but we could not get a
 * proper 48 Mhz clock for the USB if we did that.
 *
 * Lastly we can divide down SYSCLK to get the ahb, apb1, and apb2 clocks
 * The ahb value also gives us FCLK for the CPU, so we leave it alone
 * apb1 is the slow peripheral bus, must be < 50, so we divide by 2 to get 48
 * apb2 is the fast peripheral bus, must be < 100, so we leave it at 96
 *
 * This is controlled by the "conf" register.
 * After reset this register is all zeros
 *  RCC conf to start:  00000000
 *  RCC conf when done: 0000000A
 * The low 2 bits select the SYSCLK source (xx10 = PLL)
 * The next 2 bits give a status on what source is actually being used.
 * The next 4 are the AHB prescaler (0 says use SYSCLK as-is)
 * There are two 3 bit fields for the APB1 and APB2 prescaler
 * Like the AHB, a value of 0 says to use the AHB value as-is.
 *
 * ============================================================
 * What changes do we make for the F429 based "disco" board.
 * Here we have an 8 Mhz external crystal
 * See again, section 6.2 in the TRM (RM0090)
 * The datasheet says the CPU can run up to 180 Mhz
 * The game is to find a VCO less than 432 that can divide
 * to give 48 for the USB and yield the highest CPU.
 * Use VCO of 336, cpu of 168.
 * Set m = 4 and n = 168 to get the 336 vco
 * Set p = 2 to SYSCLK is 168
 * Set q = 7 so USB is 336/7 = 48
 * We let AHB remain at 168
 * the fast apb2 must < 90 -- divide by 2 to get 84
 * the slow apb1 must < 45 -- divide by 4 to get 42
 *
 * ============================================================
 * What changes do we make for the F407 based Olimex E407
 * Here we have an 12 Mhz external crystal
 */

#ifdef CHIP_F429
#define CPU_NAME		"F429"
#define PCLK1           42000000
#define PCLK2           84000000
#define CPU_HZ          168000000

#elif CHIP_F407
/* XXX */
#define CPU_NAME		"F407"
#define PCLK1           48000000
#define PCLK2           96000000
#define CPU_HZ          96000000

#else
/* Must be last */
#define CPU_NAME		"F411"
#define PCLK1           48000000
#define PCLK2           96000000
#define CPU_HZ          96000000
#endif

/* ------------------------------------------------------------- */
/* Here is code just for the F411 black pill */

#define PLL_M_VAL	25			/* 25/25 feeds 1 Mhz to PLL */

/* Yes, we really do just shift in the actual value we want
 * range is 50 to 432
 */
#define PLL_N_192	(192<<PLL_N_SHIFT)	/* PLL yiels 192 Mhz */
#define PLL_N_128	(128<<PLL_N_SHIFT)	/* PLL yiels 128 Mhz */

#define PLL_Q_VAL	(4<<PLL_Q_SHIFT)	/* 192/4 = 48 Mhz to USB */

#define PLL_P_VAL_96	(PLL_P_2<<PLL_P_SHIFT)	/* 192/2 = 96 Mhz to cpu */
#define PLL_P_VAL_48	(PLL_P_4<<PLL_P_SHIFT)	/* 192/4 = 48 Mhz to cpu */

/*                      25          192          2             4       */
#define PLL_VAL_F411 ( PLL_M_VAL | PLL_N_192 | PLL_P_VAL_96 | PLL_Q_VAL )

static void
cpu_clock_init_f411 ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

	/* Turn on HSE oscillator */
	rp->cr |= CR_HSEON;
	while ( ! (rp->cr & CR_HSERDY) )
	    ;

	/* Configure PLL */
	xyz = rp->pll & PLL_RESERVED;
	xyz |= PLL_VAL_F411;
	xyz |= PLL_SRC_HSE;
	rp->pll = xyz;

	/* Turn on PLL */
	rp->cr |= CR_PLLON;
	while ( ! (rp->cr & CR_PLLRDY) )
	    ;

	/* switch from HSI to PLL */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_PLL;

	/* reduce APB1 clock to 48 Mhz */
	xyz |= (APB_DIV2<<APB1_SHIFT);
	rp->conf = xyz;
}

#define PLL_M_4		4			/* 8/4 feeds 2 Mhz to PLL */
#define PLL_N_168	(168<<PLL_N_SHIFT)
#define PLL_Q_7		(7<<PLL_Q_SHIFT)	/* 2*168/7 = 48 Mhz to USB */

/*                      4          168          2             7       */
#define PLL_VAL_F429 ( PLL_M_4 | PLL_N_168 | PLL_P_DIV_2 | PLL_Q_7 )
static void
cpu_clock_init_f429 ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

	/* Turn on HSE oscillator */
	rp->cr |= CR_HSEON;
	while ( ! (rp->cr & CR_HSERDY) )
	    ;

	/* Configure PLL */
	xyz = rp->pll & PLL_RESERVED;
	xyz |= PLL_VAL_F429;
	xyz |= PLL_SRC_HSE;
	rp->pll = xyz;

	/* Turn on PLL */
	rp->cr |= CR_PLLON;
	while ( ! (rp->cr & CR_PLLRDY) )
	    ;

	/* switch from HSI to PLL */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_PLL;

	/* MCO2 gives us SYSCLK on PC9 */
	/* MCO1 gives us PLL on PA8 */
	xyz |= MCO1_PLL;

	/* reduce slow APB1 clock to 42 Mhz */
	xyz |= (APB_DIV4<<APB1_SHIFT);
	/* reduce fast APB2 clock to 84 Mhz */
	xyz |= (APB_DIV2<<APB2_SHIFT);
	rp->conf = xyz;
}

static void
cpu_clock_init ( void )
{
#ifdef CHIP_F411
	cpu_clock_init_f411 ();
#else
	cpu_clock_init_f429 ();
	/* XXX */
#endif
}

/* ================================================================ */
/* ================================================================ */

/* Note that only GPIO A,B,C are wired to pins on the F411, so it is
 * pointless to power up D,E,H
 */
static void
rcc_bus_init ( void )
{
	struct rcc *rp = RCC_BASE;

	/* The F411 chip powers up with resets not being asserted,
	 *  so we don't need to do anything with resets here.
	 * We do need to enable all the clocks though.
	 */
	rp->ahb1_e |= GPIOA_ENABLE;
	rp->ahb1_e |= GPIOB_ENABLE;
	rp->ahb1_e |= GPIOC_ENABLE;

	rp->apb1_e |= UART2_ENABLE;

	/* This is the FS OTG USB, which is
	 * the only one on the F411.
	 */
	rp->ahb2_e |= USB_ENABLE;
	rp->ahb2_elp |= USB_ENABLE;

	/* This is the additional HS OTG USB,
	 * which is present in the F429 and is the
	 * one being used on the Discovery board.
	 * first the clock, then the clock in low power.
	 */
	rp->ahb1_e |= USB_HS_ENABLE;
	rp->ahb1_elp |= USB_HS_ENABLE;

	rp->apb2_e |= UART1_ENABLE;
	rp->apb2_e |= UART3_ENABLE;
	rp->apb2_e |= SYSCFG_ENABLE;
}

void
rcc_init ( void )
{

	cpu_clock_init ();
	rcc_bus_init ();
}


#ifdef OLD_F411

// ===========================
// ===========================
// ===========================
// Pick one of the following

// #define CLOCK_16	// no init, run from HSI at default
// #define CLOCK_32	// hsi doubled by PLL
// #define CLOCK_HSI_96
//
// #define CLOCK_25
// #define CLOCK_48
// #define CLOCK_96

#ifdef CHIP_F407
// Olimex E407 with F407
#define CLOCK_48
#elif CHIP_F429
// my disco board
#define CLOCK_32
#else
// for the F411
#define CLOCK_96
#endif

#ifdef CLOCK_48
#define PLL_VAL ( PLL_M_VAL | PLL_N_192 | PLL_P_VAL_48 | PLL_Q_VAL )
#else	/* CLOCK_96 */
#define PLL_VAL ( PLL_M_VAL | PLL_N_192 | PLL_P_VAL_96 | PLL_Q_VAL )
#endif

/* For an experiment, see if we can run 16 in and out of the PLL */
#define PLL_16 ( 16 | PLL_N_128 | PLL_P_DIV_8 | PLL_Q_VAL )

#define PLL_32 ( 16 | PLL_N_128 | PLL_P_DIV_4 | PLL_Q_VAL )
#define PLL_96 ( 16 | PLL_N_192 | PLL_P_DIV_2 | PLL_Q_VAL )

/* The PLL is an alternate 3rd clock source.
 * We would like to run the CPU at 100 Mhz and
 * the USB clock at 48 Mhz, but this cannot be done.
 * But we can have the CPU at 96 Mhz and the proper
 * 48 Mhz USB, or we could run the CPU at 100 and
 * let the USB run at 50 (not good, but if you don't
 * intend to use the USB, why not.
 */
static void
cpu_clock_init_pll ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

	/* Turn on HSE oscillator */
	rp->cr |= CR_HSEON;
	while ( ! (rp->cr & CR_HSERDY) )
	    ;

	/* Configure PLL */
	xyz = rp->pll & PLL_RESERVED;
	xyz |= PLL_VAL;
	xyz |= PLL_SRC_HSE;
	rp->pll = xyz;

	/* Turn on PLL */
	rp->cr |= CR_PLLON;
	while ( ! (rp->cr & CR_PLLRDY) )
	    ;

	/* switch from HSI to PLL */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_PLL;

	/* reduce APB1 clock to 48 Mhz */
#ifdef CLOCK_96
	xyz |= (APB_DIV2<<APB1_SHIFT);
#endif
	rp->conf = xyz;
}

/* Here we just switch to the PLL using the
 * "on reset" values, which should give
 * 96 Mhz
 */
static void
cpu_clock_init_hsi ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

#ifdef ENSURE_HSI
	/* Configure PLL */
	xyz = rp->pll & PLL_RESERVED;
	// xyz |= PLL_16;
	// xyz |= PLL_32;
	xyz |= PLL_96;
	xyz |= PLL_SRC_HSI;
	rp->pll = xyz;
#endif

	/* Turn on PLL */
	rp->cr |= CR_PLLON;
	while ( ! (rp->cr & CR_PLLRDY) )
	    ;

	/* switch from HSI to PLL */
	/* The APB1 bus must run no faster than 50,
	 * The APB2 bus can run up to 100.
	 */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_PLL;
	xyz |= (APB_DIV2<<APB1_SHIFT);
	// show_reg ( "CONF is:", &rp->conf );
	// show32 ( "CONF will be:", xyz );
	rp->conf = xyz;
}

static void
cpu_clock_init_32 ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

	/* Configure PLL */
	xyz = rp->pll & PLL_RESERVED;
	// xyz |= PLL_16;
	xyz |= PLL_32;
	xyz |= PLL_SRC_HSI;
	rp->pll = xyz;

	/* Turn on PLL */
	rp->cr |= CR_PLLON;
	while ( ! (rp->cr & CR_PLLRDY) )
	    ;

	/* switch from HSI to PLL */
	/* The APB1 bus must run no faster than 50,
	 * The APB2 bus can run up to 100.
	 */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_PLL;
	// xyz |= (APB_DIV2<<APB1_SHIFT);
	rp->conf = xyz;
}

/* Some initial attempts locked up the chip.
 * Recovery required holding down BOOT0 and
 * doing reset, then SWD reflash would work.
 *
 * WeAct schematic shows a crystal on HSE,
 *  not a crystal oscillator.
 * This is apparently correct for my board.
 */
static void
cpu_clock_init_25 ( void )
{
	struct rcc *rp = RCC_BASE;
	unsigned int xyz;

	/* Turn on HSE oscillator */
	rp->cr |= CR_HSEON;
	while ( ! (rp->cr & CR_HSERDY) )
	    ;

	/* switch from HSI to HSE */
	xyz = rp->conf;
	xyz &= ~CONF_CLOCK_BITS;
	xyz |= CONF_HSE;
	rp->conf = xyz;
}

/* The RM for the F411 has a table in section 3 for various voltages and
 * processor speeds that indicates how many flash wait states
 * are required.  I always run at 3.3 volts, so the rules
 * are as follows:
 *
 *  0-30 Mhz 0 waits
 * 30-64 Mhz 1 wait
 * 64-90 Mhz 2 waits
 * 90-100 Mhz 3 waits
 */
static void
cpu_clock_init ( void )
{
#ifdef CLOCK_25
	cpu_clock_init_25 ();
#endif
#if defined(CLOCK_96)
	flash_init ( 3 );
	cpu_clock_init_pll ();
#endif
#if defined(CLOCK_48)
	flash_init ( 1 );
	cpu_clock_init_pll ();
#endif
#ifdef CLOCK_HSI_96
	flash_init ( 3 );
	cpu_clock_init_hsi ();
#endif
#ifdef CLOCK_32
	flash_init ( 1 );
	cpu_clock_init_32 ();
#endif
}

#ifdef notdef
/* Hook so we can use serial IO for debug
 * later after serial is initialized.
 */
void
rcc_debug ( void )
{
	// cpu_clock_init_xxx ();

}
#endif

/* On the Black Pill boards that I have, we have an external
 * 25 Mhz crystal.
 * We will multiply that by 4 someday, somehow to get 100 Mhz.
 * Actually we have to settle for 96 Mhz if we want
 * a proper USB clock.
 *
 * Like the F103, the APB1 domain cannot run full speed like
 *  the APB2 can.  On the F411, 50 Mhz is the limit.
 *
 * There is also a 16 Mhz RC internal oscillator (HSI)
 * The chip fires up using it.
 */

#ifdef CLOCK_16
#define PCLK1           16000000
#define PCLK2           16000000
#define CPU_HZ          16000000
#endif

/* Correct for F429, uart1 */
#ifdef CLOCK_32
#define PCLK1           32000000
#define PCLK2           32000000
#define CPU_HZ          32000000
#endif

#ifdef CLOCK_HSI_96
#define PCLK1           48000000
#define PCLK2           96000000
#define CPU_HZ          96000000
#endif

#ifdef CLOCK_25
#define PCLK1           25000000
#define PCLK2           25000000
#define CPU_HZ          25000000
#endif

#ifdef CLOCK_48_ORIG
#define PCLK1           48000000
#define PCLK2           48000000
#define CPU_HZ          48000000
#endif

/* for F407, correct for UART1 anyway */
#ifdef CLOCK_48
#define PCLK1           48000000
#define PCLK2           24000000	// used by UART1
#define CPU_HZ          48000000
#endif

/* Correct for F411 */
#ifdef CLOCK_96
#define PCLK1           48000000
#define PCLK2           96000000
#define CPU_HZ          96000000
#endif

#endif	/* OLD_F411 */

char *
get_chip_name ( void )
{
	// return CPU_NAME;
	return "SAM";
}

int
get_cpu_hz ( void )
{
	return CPU_HZ;
}

/* These will differ if we run with a clock over 50 Mhz, as
 * PCLK1 must not exceed 50.
 */
int
get_pclk1 ( void )
{
        return PCLK1;
}

int
get_pclk2 ( void )
{
        return PCLK2;
}

/* We cannot use printf inside of rcc_init() as the serial port
 * is not yet set up.
 */
void
rcc_show ( void )
{
	struct rcc *rp = RCC_BASE;

	// Shows all zeros
	// printf ( "RCC conf to start: %X\n", conf_orig );
	printf ( "RCC conf when done: %X\n", rp->conf );
	printf ( "%s cpu running at %d Mhz\n", get_chip_name(), get_cpu_hz() );
	printf ( " Pclk1 (slow) = %d Mhz\n", get_pclk1() );
	printf ( " Pclk2 (fast) = %d Mhz\n", get_pclk2() );
}

/* THE END */
