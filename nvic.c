/* Nvic.c
 * (c) Tom Trebisky  7-7-2017
 *
 * Driver for the STM32F103 NVIC interrupt controller
 *
 * The registers for this are part of the ARM Cortex M3
 *  "System Control Space" which contains:
 *
 *  0xE000E000 - interrupt type register
 *  0xE000E010 - system timer (SysTick)
 *  0xE000E100 - NVIC (nested vectored interrupt controller)
 *  0xE000ED00 - system control block (includes CPU ID)
 *  0xE000EF00 - software trigger exception registers
 *  0xE000EFD0 - ID space
 */

#include "hydra.h"

struct nvic {
	volatile unsigned int iser[8];	/* 00 set enable */
	int __pad1[24];
	volatile unsigned int icer[8];	/* 80 clear enable */
	int __pad2[24];
	volatile unsigned int ispr[8];	/* 100 set pending */
	int __pad3[24];
	volatile unsigned int icpr[8];	/* 180 clear pending */
	int __pad4[24];
	volatile unsigned int iabr[8];	/* 200 active bit */
	int __pad5[56];
	volatile unsigned char ip[240];	/* 300 priority */
	int __pad6[644];
	volatile unsigned int stir;	/* EF00 - software trigger */
};

#define NVIC_BASE	((struct nvic *) 0xe000e100)

// For an F407 I define both 411 and 407
// Maybe we should just set the limit to 91 for
// all chips and be done with it?

#ifdef CHIP_F103
 #define NUM_IRQ	60
#endif

#if defined(CHIP_F407) || defined(CHIP_F429)
 #define NUM_IRQ	91
#else	/* just F411 */
 #define NUM_IRQ	68
#endif

/* I don't do anything to initialize the NVIC and this works just fine
 *  for what I do.  Some people might want to fiddle with priorities.
 */
void
nvic_init ( void )
{
	// struct nvic *np = NVIC_BASE;

	// Be sure the serial IO system is initialized
	//  before printing from here.
	// show_reg ( "nvic stir", &np->stir );
	printf ( "Nvic initialized with %d IRQ\n", NUM_IRQ );
}

void
nvic_enable ( int irq )
{
	struct nvic *np = NVIC_BASE;

	if ( irq >= NUM_IRQ ) {
		printf ( "Nvic, IRQ %d out of range\n", irq );
	    return;
	}

	np->iser[irq/32] = 1 << (irq%32);
}

/* THE END */
