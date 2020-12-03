/* flash.c
 * (c) Tom Trebisky  12-2-2020
 *
 * Flash memory control for Hydra
 */

#ifdef CHIP_F411
struct flash {
	volatile unsigned int acr;
	volatile unsigned int keyr;
	volatile unsigned int optkeyr;
	volatile unsigned int sr;
	volatile unsigned int cr;
	volatile unsigned int optcr;
};

#define FLASH_BASE (struct flash *) 0x40023c00

/* In the ACR */
#define FL_PRFTEN	0x100
#define FL_ICEN		0x200
#define FL_DCEN		0x400
#define FL_ICRST	0x800
#define FL_DCRST	0x1000

static void
flash_waits ( int nw )
{
	struct flash *fp = FLASH_BASE;
	unsigned int xyz;

	xyz = fp->acr;
	xyz &= ~0xf;
	xyz |= nw;
	fp->acr = xyz;
}

/* Enable data cache, instruction cache, and prefetch */
static void
flash_cache_enable ( void )
{
	struct flash *fp = FLASH_BASE;

	fp->acr |= FL_PRFTEN | FL_ICEN | FL_DCEN;
}
#endif

/* ====================================================================== */

#ifdef CHIP_F103
struct flash {
	volatile unsigned int acr;
};

#define FLASH_BASE (struct flash *) 0x40022000

#define FLASH_PF_STATUS 0x0020  /* prefetch status */
#define FLASH_PRFTEN    0x0010  /* enable prefetch buffer */
#define FLASH_HCYCLE    0x0008  /* enable half cycle access */

#define FLASH_WAIT0     0  /* for sysclk <= 24 Mhz */
#define FLASH_WAIT1     1  /* for 24 < sysclk <= 48 Mhz */
#define FLASH_WAIT2     2  /* for 48 < sysclk <= 72 Mhz */

static void
flash_waits ( int nw )
{
	struct flash *fp = FLASH_BASE;
	unsigned int xyz;

	xyz = fp->acr;
	xyz &= ~0x7;
	xyz |= nw;
	fp->acr = xyz;
}

/* Enable prefetch */
static void
flash_cache_enable ( void )
{
	struct flash *fp = FLASH_BASE;

	fp->acr |= FLASH_PRFTEN;
	// * FLASH_ACR = FLASH_PRFTEN | FLASH_WAIT2;
}
#endif

/* ====================================================================== */

void
flash_init ( int nw )
{
	flash_waits ( nw );
	flash_cache_enable ();
}

/* THE END */
