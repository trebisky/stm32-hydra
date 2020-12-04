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

#ifdef CHIP_F103
 #define NUM_IRQ	60
#else
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
}

void
nvic_enable ( int irq )
{
	struct nvic *np = NVIC_BASE;

	if ( irq >= NUM_IRQ )
	    return;

	np->iser[irq/32] = 1 << (irq%32);
}

/* THE END */
