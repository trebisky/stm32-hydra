/* A bare start to a USB driver.
 *  - for now just placeholder stubs for
 *  entries I have added to locore.s
 * Tom Trebisky  12-6-2020
 */

#include "hydra.h"

/* Remember that for us, as a device, the IN endpoint is viewed
 * from the perspective of the host.  So our IN endpoint is
 * actually sending data.  And correspondingly, our OUT endpoint
 * is receiving data.  A potential point of confusion to be
 * aware of.
 */

/* Note - OUT endpoint section has no status register */
struct usb_endpoint {
	/* device - IN endpoint registers */
	volatile unsigned int ctl;		/* 0x900 control */
	    int _pad0;
	volatile unsigned int ir;		/* 0x908 interrupt */
	    int _pad1;
	volatile unsigned int size;		/* 0x910 Xfer size */
	volatile unsigned int dma;		/* 0x914 DMA address */
	volatile unsigned int status;		/* 0x918 Tx fifo status (available space) */
	    int _pad2;
};

#define NUM_EP	4

struct usb_hw {
	/* Core registers */
	volatile unsigned int csr;	/* 0x000 GOTGCTL */
	volatile unsigned int ir;	/* 0x004 GOTGINT */
	volatile unsigned int ahb_conf;	/* 0x008 GAHBCFG */
	volatile unsigned int usb_conf;	/* 0x00c GUSBCFG */
	volatile unsigned int reset;	/* 0x010 GRSTCTL */
	volatile unsigned int is;	/* 0x014 GINTSTS - int status */
	volatile unsigned int im;	/* 0x018 GINTMSK - int mask */
	volatile unsigned int rx;	/* 0x01c GRXSTSR - Rx status debug read */
	volatile unsigned int rxp;	/* 0x020 GRXSTSP - Rx status read and pop */
	volatile unsigned int rx_fifo;	/* 0x024 GRXFSIZ - Rx fifo size*/
	volatile unsigned int np_fifo;	/* 0x028 DIEPTXF0 - Tx fifo for EP0 in device mode */
	volatile unsigned int qstat;	/* 0x02C HNPTXSTS */
	    int _pad0[2];
	volatile unsigned int conf;	/* 0x038 GCCCFG - core config*/
	volatile unsigned int cid;	/* 0x03C CID - core ID */
	    int _pad1[64-16];
	volatile unsigned int hp_fifo;		/* 0x100 HNPTXSTS - looks host only*/
	volatile unsigned int in_fifo[3];	/* 0x104 DIEPTXFx - for IN 1,2,3 */
	    int _pad2[64*3-4];

	/* Host mode registers.
	 * I don't plan to use this, so I am only
	 * setting up the first one as a place holder.
	 * See the RM for the rest if you ever get
	 * interested.
	 */
	volatile unsigned int hcfg;		/* 0x400 host config */
	    int _pad3[64*4-1];

	/* Device mode registers.
	 */
	volatile unsigned int dcfg;		/* 0x800 device config */
	volatile unsigned int dctl;		/* 0x804 device control */
	volatile unsigned int dstat;		/* 0x808 device status */
	    int _pad4;
	volatile unsigned int in_imask;		/* 0x810 IN endpoint interrupt mask */
	volatile unsigned int out_imask;	/* 0x814 OUT endpoint interrupt mask */
	volatile unsigned int all_int;		/* 0x818 all endpoints interrupt reg */
	volatile unsigned int all_mask;		/* 0x81c all endpoints interrupt mask */
	    int _pad5[2];
	volatile unsigned int v_dis;		/* 0x828 Vbus discharge time */
	volatile unsigned int v_pulse;		/* 0x82c Vbus pulse time */
	    int _pad6;
	volatile unsigned int in_fifo_empty;	/* 0x834 IN fifo empty interrupt mask */
	    int _pad7[64-14];

	/* device - IN endpoint registers */
	struct usb_endpoint in_ep[NUM_EP];	/* 0x900 */
	    int _pad8[128-4*8];

	/* device - OUT endpoint registers */	/* 0xB00 */
	struct usb_endpoint out_ep[NUM_EP];
	    int _pad9[128+64-4*8];

	/* power and clock gating */
	volatile unsigned int clock;		/* 0xE00 - power and clock gating */

	    int _pad10[2*64-1];

	struct {
	    volatile unsigned int reg;		/* 0x1000, 0x2000, ... */
	    int _pad[1023];
	} fifo[4];
};

/* clear a bit or field, then set a value in it.
 * This works nicely, and if the val is zero, the compiler
 * optimizes the OR away entirely.
 */
#define usb_mod_reg(reg,mask,val)	hp->reg = (hp->reg & ~mask) | val

/* And these set or clear -- not that I will necessarily ever use them */
// #define usb_set_reg(reg,val)		hp->reg = val
#define usb_clear_bits(reg,mask)	hp->reg &= ~mask
#define usb_set_bits(reg,mask)		hp->reg |= mask

/* bits in the USB conf register */
#define CONF_DFORCE	BIT(30)	/* force device mode */
#define CONF_HFORCE	BIT(29)	/* force host mode */
#define CONF_PHY	BIT(6)	/* PHY select, always 1, ignore */
#define CONF_HNP	BIT(9)	/* enable HNP (for OTG) */
#define CONF_SRP	BIT(8)	/* enable SRP (for OTG) */

/* bits in the AHB config register */
#define AHB_GINT	1	/* global interrupt enable */

/* bits in the interrupt and interrupt mask register */
#define INT_CMOD	1	/* current mode of operation */
#define INT_MMIS	BIT(1)	/* mode mismatch */
#define INT_OTG		BIT(2)	/* OTG (read OTG interrupt register) */
#define INT_SOF		BIT(3)
#define INT_RXFLVL	BIT(4)	/* Rx FIFO not empty - something to read */
/* */
#define INT_ESUSP	BIT(10)	/* early suspend detected */
#define INT_SUSP	BIT(11)	/* suspend detected */
#define INT_RESET	BIT(12)	/* reset detected */
#define INT_ENUM	BIT(13)	/* enum done */
#define INT_IN_EP	BIT(18)	/* IN endpoint */
#define INT_OUT_EP	BIT(19)	/* OUT endpoint */
#define INT_ISOIN	BIT(20) /* Incomplete ISO in */
#define INT_ISOOUT	BIT(21) /* Incomplete ISO out */
#define INT_WKUP	BIT(31)	/* OUT endpoint */

/* .. and more .. */

/* bits in the reset register */
#define RESET_AHB_IDLE	BIT(31)
#define RESET_TX_SHIFT	6
#define RESET_FIFO_ALL	0x10

#define RESET_TX_FL	BIT(5)	/* Flush TxFIFO */
#define RESET_RX_FL	BIT(4)	/* Flush entire RxFIFO */
#define RESET_FC	BIT(2)	/* Frame counter reset (host only) */
#define RESET_HCLK	BIT(1)	/* HCLK soft reset */
#define RESET_CORE	1	/* Core soft reset */

/* bits in the general core config register (conf) 0x38 */
#define CONF_NOVBUS	BIT(21)	/* set to disble Vbus sensing on special pin */
#define CONF_SOF	BIT(20)	/* set to enable SOF on special pin */
#define CONF_A_SENSE	BIT(19)	/* set to enable Vbus A sensing */
#define CONF_B_SENSE	BIT(18)	/* set to enable Vbus B sensing */
#define CONF_PWRDOWN	BIT(16)	/* set to deactivate power down */

/* Bits in the Device config register. */

#define DCONF_FI_MASK		(0x3<<11)
#define DCONF_FI_80		0x0000
#define DCONF_DAD_MASK		(0x7F<<4)
#define DCONF_SPEED_MASK	0x0003
#define DCONF_SPEED_FS		0x0003

/* Bits in the Device control register */
#define DCTL_RWU		1	/* remote wakeup signal */
#define DCTL_SDIS		BIT(1)	/* soft disconnect */
/* Set the SDIS bit to perform a soft disconnect, clear it to run */
#define DCTL_GINAK		BIT(2)	/* Global IN Nak status (RO) */
#define DCTL_GONAK		BIT(3)	/* Global OUT Nak status (RO) */
#define DCTL_SGINAK		BIT(7)	/* Set Global IN Nak */
#define DCTL_CGINAK		BIT(8)	/* Clear Global IN Nak */
#define DCTL_SGONAK		BIT(9)	/* Set Global OUT Nak */
#define DCTL_CGONAK		BIT(10)	/* Clear Global OUT Nak */
#define DCTL_POPDONE		BIT(11)	/* Power on programming done */

/* Bits in the device IN endpoint interrupt mask register */
#define DIN_NAKM	BIT(13)		/* Nak */
#define DIN_NEM		BIT(6)		/* Nak effective */
#define DIN_MM		BIT(5)		/* EP mismatch */
/* On some hardware, the above may be a fifo underrun bit */
#define DIN_TXE		BIT(4)		/* token when Tx empty */
#define DIN_TO		BIT(3)		/* timeout */
#define DIN_DIS		BIT(1)		/* endpoint disabled */
#define DIN_CM		BIT(0)		/* transfer complete */

/* Bits in the endpoint control register */
#define EP_CTL_ENA	BIT(31)
#define EP_CTL_DIS	BIT(30)
#define EP_CTL_SNAK	BIT(27)
#define EP_CTL_CNAK	BIT(26)
#define EP_CTL_FIFO_MASK	(0xf<<22)
#define EP_CTL_FIFO_SHIFT	22
#define EP_CTL_AEP	BIT(15)		/* Active Endpoint */


/* --------------------- */

#define IRQ_USB_WAKEUP	42
#define IRQ_USB_FS	67

/* Register base */
#define USB_BASE	(struct usb_hw *) 0x50000000
// #define USB_FIFO	(unsigned int *) 0x50001000

// static unsigned int *usb_fifo_base[] =
//     { (unsigned int *) 0x50001000, (unsigned int *) 0x50002000,
//     (unsigned int *) 0x50003000, (unsigned int *) 0x50004000 };

#define USB_FIFO	(unsigned int *) 0x50001000

/* Direct access here only for debugging */
#define USB_RAM_BASE	0x50020000
#define USB_RAM_SIZE	1280		/* in bytes */
#define USB_RAM_WSIZE	320		/* in 32 bit words */

/* ============================================================== */
/* Prototypes */

static void flush_tx_fifo ( int );

/* ============================================================== */

#define FIFO_FILL	0xABABABAB

static void
fill_fifo_ram ( void )
{
	unsigned int *p = (unsigned int *) USB_RAM_BASE;
	int i;

	for ( i=0; i< USB_RAM_WSIZE; i++ )
	    *p++ = FIFO_FILL;
}

/* Count in lines of 4 numbers */
static void
dump_l ( void *addr, int n )
{
        unsigned int *p;
        int i;

        p = (unsigned int *) addr;

        while ( n-- ) {
            printf ( "%X  ", (long) addr );

            for ( i=0; i<4; i++ )
                printf ( "%X ", *p++ );

            printf ( "\n" );
            addr += 16;
        }
}

static unsigned int ram_snapshot[USB_RAM_WSIZE];

// void *memcpy(void *dest, const void *src, size_t n);
// void *memcpy(void *dest, void *src, int n);

static void
dump_fifo_ram ( void )
{
	unsigned int *p, *q;
	int n;

	n = USB_RAM_WSIZE;
	q = (unsigned int *) USB_RAM_BASE;
	p = ram_snapshot;

	while ( n-- )
	    *p++ = *q++;

	// memcpy ( (void *)ram_snapshot, (void *) USB_RAM_BASE, USB_RAM_SIZE );
	dump_l ( (void *) ram_snapshot, USB_RAM_SIZE / sizeof(long) / 4 );
}

/* Returns 0 if FIFO still matches fill,
 * else number of mismatches.
 */
static int
check_fifo_ram ( void )
{
	unsigned int *p = (unsigned int *) USB_RAM_BASE;
	int i;
	int rv = 0;

	for ( i=0; i< USB_RAM_WSIZE; i++ )
	    if ( *p++ != FIFO_FILL )
		++rv;
	return rv;
}

/* ============================================================== */

static void
show_in_ep ( int ep )
{
	struct usb_hw *hp = USB_BASE;

	printf ( "IN ep %d\n", ep );
	show_reg ( "IN ep ctl: ", &hp->in_ep[ep].ctl );
	show_reg ( "IN ep size: ", &hp->in_ep[ep].size );
	show_reg ( "IN ep status: ", &hp->in_ep[ep].status );
}

/* Called by enum done */
static void
enable_ep0 ( void )
{
	struct usb_hw *hp = USB_BASE;

	/* Get enumerated speed.
	 * Should always be Full speed (Phy clock at 48) for this chip */
	if ( hp->dstat & 0x6 != 0x6 )
	    printf ( "Weird enumerated speed: %x\n", hp->dstat );

	/* A more general driver would set the MPS bits here.
	 * setting them to zero gives a max packet size of 64,
	 * which is what we want and have already.
	 */

	/* A more general driver would also set the speed bits */
	// hp->in_ep[0] &= ~0x3;

	/* Clear global IN Nak flag */
	hp->dctl |= DCTL_CGINAK;
	// show_reg ( "enable_ep0; dctl: ", &hp->dctl );
}

/* Called by reset */
static void
start_ep0_out ( void )
{
	struct usb_hw *hp = USB_BASE;

#define EP_OUT_SETUP_1		(1<<29)
#define EP_OUT_SETUP_2		(2<<29)
#define EP_OUT_SETUP_3		(3<<29)
#define EP_IN_PKTCNT_SHIFT	19
#define EP_OUT_PKTCNT_SHIFT	19
#define EP_OUT_PKTCNT		(1 << 19)

	/* We allow 3 back to back setup packets to be received.
	 */
	hp->out_ep[0].size = EP_OUT_SETUP_3 | EP_OUT_PKTCNT | 8*3;

	/* Make the endpoint active */
	hp->out_ep[0].ctl = EP_CTL_ENA | EP_CTL_AEP;
}

static void
start_ep_in ( int ep )
{
	struct usb_hw *hp = USB_BASE;

	/* We allow 3 back to back setup packets to be received.
	 */
	// hp->out_ep[0].size = EP_OUT_SETUP_3 | EP_OUT_PKTCNT | 8*3;

	/* Make the endpoint active */
	hp->in_ep[ep].ctl = EP_CTL_ENA | EP_CTL_AEP;
}

/* Handle enum interrupt.
 *  In general, this comes close on the heels of a reset interrupt.
 */
static void
handle_enum ( void )
{
	enable_ep0 ();
	show_in_ep ( 0 );
	start_ep_in ( 0 );
	show_in_ep ( 0 );
}

/* Handle reset interrupt */
static void
handle_reset ( void )
{
	struct usb_hw *hp = USB_BASE;
	int i;

	/* Clear the remote wakeup bit */
	hp->dctl &= ~DCTL_RWU;

	/* Flush the IN fifo for EP0 */
	flush_tx_fifo ( 0 );

	for ( i=0; i<NUM_EP; i++ ) {
	    hp->in_ep[i].ir = 0xff;
	    hp->out_ep[i].ir = 0xff;
	}

	/* Clear all */
	hp->all_int = 0xffffffff;

	/* Enable EP0 interrupts */
	hp->all_mask = (1 << 16) | 1;

#define OUT_EP_INT_XFERCM	BIT(0)
#define OUT_EP_INT_DIS		BIT(1)
#define OUT_EP_INT_SETUP	BIT(3)
	hp->out_imask = OUT_EP_INT_XFERCM | OUT_EP_INT_SETUP | OUT_EP_INT_DIS;

#define IN_EP_INT_XFERCM	BIT(0)
#define IN_EP_INT_DIS		BIT(1)
#define IN_EP_INT_TO		BIT(3)
#define IN_EP_INT_EMPTY		BIT(7)	/* Empty (not in F411) */
#define IN_EP_INT_TXU		BIT(8)	/* Tx underrun (not in F411) */
	hp->in_imask = IN_EP_INT_XFERCM | IN_EP_INT_TO | IN_EP_INT_DIS;

	/* Set device address to 0 */
	hp->dcfg &= ~DCONF_DAD_MASK;

	start_ep0_out ();
}

static unsigned int sof_count = 0;
static int lne = 0;

/* Dump some registers, triggered by the
 * SOF "clock" timing.
 */
static void
sof_dump ( void )
{
	struct usb_hw *hp = USB_BASE;

	printf ( "----- SOF = %d\n", sof_count );
	show_reg ( "all EP int: ", &hp->all_int );
	show_reg ( "EP0 IN ints: ", &hp->in_ep[0].ir );
	show_reg ( "EP0 OUT ints: ", &hp->out_ep[0].ir );
}

static void
handle_sof ( void )
{
	    ++sof_count;

	    /*
	    if ( sof_count == 1 )
		sof_dump ();
	    if ( sof_count == 5000 )
		sof_dump ();
	    */
}

static void
handle_out ( void )
{
	struct usb_hw *hp = USB_BASE;
	int s;

	// s = hp->all_int;
	show_reg ( "OUT all reg: ", &hp->all_int );
	show_reg ( "OUT EP0 int reg: ", &hp->out_ep[0].ir );
	hp->out_ep[0].ir = 0xff;
}

/* single line byte dump */
static void
dump_b ( char *msg, char *buf, int len )
{
	int i;

	printf ( "%s", msg );

	for ( i=0; i<len; i++ ) {
	    printf ( " %x", buf[i] );
	}

	printf ( "\n" );
}

/* Bits in rx/rxp register */
#define RX_EP_MASK	0xf
#define RX_CNT_SHIFT	4
#define RX_CNT_MASK	0x7ff
#define RX_PID_SHIFT	15
#define RX_PID_MASK	0x3
#define RX_STAT_SHIFT	17
#define RX_STAT_MASK	0xf

static void
read_setup ( void )
{
	struct usb_hw *hp = USB_BASE;
	// volatile unsigned int *fifo = usb_fifo_base[0];
	volatile unsigned int *fifo = &hp->fifo[0].reg;
	unsigned int buf[2];
	unsigned int stat;
	int count;
	int ep;
	int st, pid;

	stat = hp->rxp;
	count = (stat >> RX_CNT_SHIFT) & RX_CNT_MASK;
	ep = stat & RX_EP_MASK;
	st = (stat >> RX_STAT_SHIFT) & RX_STAT_MASK;
	pid = (stat >> RX_PID_SHIFT) & RX_PID_MASK;

	printf ( "Setup status (rxp): %X %d bytes on EP %d (%x %x)\n", stat, count, ep, st, pid );

	if ( count == 0 )
	    return;

	buf[0] = *fifo;
	buf[1] = *fifo;
	dump_b ( "Setup: ", (char *) buf, 8 );
	usb_console_setup ( buf );

	// printf ( "Setup: %X %X\n", val1, val2 );
}

static void
usb_write_ep ( int ep, char *buf, int len )
{
	// volatile unsigned int *fifo = usb_fifo_base[ep];
	volatile unsigned int *fifo = &(USB_BASE)->fifo[ep].reg;
	int wlen = (len+3) / 4;

	while ( wlen-- ) {
	    *fifo = * (unsigned int *) buf;
	    buf += sizeof(unsigned int);
	}
}

/* Write a response from a setup packet */
void
usb_write_data ( char *buf, int len )
{
	struct usb_hw *hp = USB_BASE;
	volatile unsigned int *fifo;

	printf ( "Write data, %d bytes\n", len );

	/* XXX - hackish */
	hp->in_ep[0].size = len | 1<<EP_IN_PKTCNT_SHIFT;
	usb_write_ep ( 0, buf, len );
	show_in_ep ( 0 );

#ifdef notdef
	fifo = &hp->fifo[0].reg;
	printf ( "FIFO at %X\n", fifo );

	// fifo = &hp->fifo[3].reg;
	fifo = &(USB_BASE)->fifo[3].reg;
	printf ( "FIFO at %X\n", fifo );
#endif
}

/* ============================================================== */

/* USB wakeup handler
 * hook in locore.s
 */
void
usb_wakeup_handler ( void )
{
	/* I've never seen this one yet.
	 */
	printf ( "USB wakeup interrupt\n" );
}

/* Main USB interrupt handler.
 * hook in locore.s
 */
void
usb_irq_handler ( void )
{
	struct usb_hw *hp = USB_BASE;
	unsigned int status;
	int ne;

	status = hp->is;

	if ( status == 0 ) {
	    printf ( "Int -- spurious %X %d\n", status, sof_count );
	    return;
	}

	/* These are cleared just by reading their status */
	if ( status & INT_IN_EP )
	    printf ( "Int -- IN endpoint %X %d\n", status, sof_count );

	if ( status & INT_OUT_EP ) {
	    printf ( "Int -- OUT endpoint %X %d\n", status, sof_count );
	    handle_out ();
	    return;
	}

	if ( status & INT_SOF ) {
	    // printf ( "Int -- SOF\n" );
	    handle_sof ();
	    hp->is |= INT_SOF;
	    /*
	    ne = check_fifo_ram ();
	    if ( ne != lne ) {
		printf ( "Stuff %d at SOF = %d\n", ne, sof_count );
		dump_fifo_ram ();
		lne = ne;
	    }
	    */
	    /*
	    if ( sof_count == 4 ) {
		printf ( "++ ++ SOF %d ++ ++\n", sof_count );
		dump_fifo_ram ();
	    }
	    */
	    return;
	}

	if ( status & INT_RXFLVL ) {
	    printf ( "Int - Rx FIFO not empty %X %d\n", status, sof_count );
	    read_setup ();
	    hp->is |= INT_RXFLVL;
	    return;
	}

	/* Most interrupts are cleared just
	 * by the first read to the status register.
	 */
	// printf ( "USB OTG FS interrupt\n" );
	// printf ( "- USB interrupt, status: %X, %d\n", status, sof_count );
	// show_reg ( "Int status: ", &hp->is );
	// printf ( "\n" );

	if ( status & INT_ENUM ) {
	    /* This is speed enumeration */
	    printf ( "Int -- Speed enumeration done %X %d\n", status, sof_count );
	    handle_enum ();
	    hp->is |= INT_ENUM;
	    return;
	}
	if ( status & INT_RESET ) {
	    printf ( "Int -- Reset %X %d\n", status, sof_count );
	    handle_reset ();
	    hp->is |= INT_RESET;
	    return;
	}

	if ( status & INT_MMIS ) {
	    printf ( "Int -- mode mismatch %X %d\n", status, sof_count );
	    hp->is |= INT_MMIS;
	} else if ( status & INT_SUSP ) {
	    printf ( "Int -- Suspend %X %d\n", status, sof_count );
	    hp->is |= INT_SUSP;
	} else if ( status & INT_WKUP ) {
	    printf ( "Int -- Wakeup %X %d\n", status, sof_count );
	    hp->is |= INT_WKUP;
	} else
	    printf ( "- USB interrupt, status: %X, %d\n", status, sof_count );
}

#ifdef notdef
/*
 * Other F4 chips (F405/F407)  support high speed
 *  and use these interrupts.
 */
void irq_usb_hs_ep1_out ( void ) {} /* 74 */
void irq_usb_hs_ep1_in ( void ) {} /* 75 */
void irq_usb_hs_wkup ( void ) {} /* 76 */
void irq_usb_hs ( void ) {} /* 75 */
#endif

void
try_int ( void )
{
	struct usb_hw *hp = USB_BASE;
	int xyz;

	hp->im |= INT_MMIS;
	xyz = hp->hcfg;
}

static void
flush_rx_fifo ( void )
{
	struct usb_hw *hp = USB_BASE;
	int tmo;

	hp->reset = RESET_RX_FL;
	for (tmo = 250000; tmo; tmo-- ) {
	    if ( ! (hp->reset & RESET_RX_FL) )
		break;
	}

	/* Delay 3 PHY clocks */
	delay_us ( 3 );
}

static void
flush_tx_fifo ( int fifo )
{
	struct usb_hw *hp = USB_BASE;
	int tmo;

	hp->reset = RESET_TX_FL | (fifo<<RESET_TX_SHIFT);
	for (tmo = 250000; tmo; tmo-- ) {
	    if ( ! (hp->reset & RESET_TX_FL) )
		break;
	}

	/* Delay 3 PHY clocks */
	delay_us ( 3 );
}

static void
flush_tx_fifo_all ( void )
{
	flush_tx_fifo ( RESET_FIFO_ALL );
}

static void
enable_usb_ints ( void )
{
	struct usb_hw *hp = USB_BASE;

	hp->im = 0;		/* disable all */
	hp->is = 0xffffffff;	/* clear all pending */
	hp->ir = 0xffffffff;	/* clear pending OTG */

	/* Enable those we want */
	hp->im = INT_SUSP | INT_WKUP | INT_RESET |
		    INT_IN_EP | INT_OUT_EP | INT_ENUM |
		    INT_SOF | INT_RXFLVL;
		    // INT_ISOIN | INT_ISOOUT;

	printf ( "Enable USB interrupt gate\n" );
	/* Turn on the main switch */
	hp->ahb_conf |= AHB_GINT;
}

static void
show_stuff ( void )
{
	struct usb_hw *hp = USB_BASE;

	show_reg ( "USB csr", &hp->csr );
	show_reg ( "USB cid", &hp->cid );

	/* We come up in device mode, so this access yields
	 * the MMIS bit set in the IR for a mode mismatch.
	 */
	// show_reg ( "USB hcfg", &hp->hcfg );

	show_reg ( "USB clock", &hp->clock );
	show_reg ( "USB ir", &hp->ir );
	show_reg ( "USB is", &hp->is );

#ifdef notdef
	/* No change since we just came out of reset */
	hp->reset |= RESET_CORE;
	printf ( "\n" );

	show_reg ( "USB csr", &hp->csr );
	show_reg ( "USB cid", &hp->cid );
	// show_reg ( "USB hcfg", &hp->hcfg );
	show_reg ( "USB clock", &hp->clock );
	printf ( "\n" );
	show_reg ( "USB ir", &hp->ir );
	show_reg ( "USB is", &hp->is );
#endif

	printf ( "\n" );
	// printf ( "Poke a host register\n" );
	// show_reg ( "USB hcfg", &hp->hcfg );

	// delay ( 500 );

	printf ( "\n" );
	hp->ahb_conf |= AHB_GINT;
	try_int ();
	printf ( "\n" );

	delay ( 500 );
	show_reg ( "USB im", &hp->im );
	show_reg ( "USB ir", &hp->ir );
	show_reg ( "USB is", &hp->is );
	show_reg ( "USB usb_conf", &hp->usb_conf );
}

/* XXX */
static void
usb_delay_ms ( int ms )
{
	while ( ms-- )
	    delay_us ( 1000 );
}

void
usb_init ( void )
{
	struct usb_hw *hp = USB_BASE;
	// unsigned int xyz;
	int start;
	int i;

	puts ( "---------------------------\n" );
	printf ( "In USB init\n" );

	/* Disable USB interrupts */
	hp->ahb_conf &= ~AHB_GINT;

	nvic_enable ( IRQ_USB_WAKEUP );
	nvic_enable ( IRQ_USB_FS );

	fill_fifo_ram ();

	gpio_usb_init ();

	/* Restart the PHY clock in case it has stopped */
	hp->clock = 0;

	/* This turns out to be quite important.
	 * Before setting this bit, the device decided
	 * for some reason to switch to host mode
	 * shortly after being initialized.
	 */
	show_reg ( "USB usb_conf", &hp->usb_conf );
	hp->usb_conf |= CONF_DFORCE;

	// 25 ms delay required to switch modes.
	// We probably don't need to delay as we
	// are already in device mode on boot.
	usb_delay_ms ( 50 );

	// show_reg ( "USB usb_conf", &hp->usb_conf );

	/* General core config register, 0x038 */
	// deactivate power down.
	// disable Vbus sensing
	hp->conf = CONF_NOVBUS | CONF_PWRDOWN;

	show_reg ( "USB dcfg", &hp->dcfg );
	// usb_mod_reg ( dcfg, DCONF_FI_MASK, DCONF_FI_80);
	// usb_mod_reg ( dcfg, DCONF_SPEED_MASK, DCONF_SPEED_FS);
	// write all fields ensuring device address set 0
	hp->dcfg = DCONF_FI_80 | DCONF_SPEED_FS;

	/*
	xyz = hp->dcfg;
	xyz &= ~DCONF_FI_MASK;
	xyz |= DCONF_FI_80;
	xyz &= ~DCONF_SPEED_MASK;
	xyz |= DCONF_SPEED_FS;
	hp->dcfg = xyz;
	*/
	// show_reg ( "USB dcfg", &hp->dcfg );

	/* Fifo sizes are in 32 bit words.
	 * We have 1.25 kb = 1280 bytes = 320 words.
	 * The RM says that we get a single Rx FIFO
	 * and several Tx FIFO's
	 * The Rx FIFO handles all OUT endpoints
	 * Each IN endpoint has its own Tx FIFO
	 * See 22.11.1 and 22.11.2 and 22.13.1
	 */

#define RX_FIFO_SIZE	128
#define TX0_FIFO_SIZE	64
#define TX1_FIFO_SIZE	128
#define TX2_FIFO_SIZE	0
#define TX3_FIFO_SIZE	0

	start = 0;
	hp->rx_fifo = RX_FIFO_SIZE;	/* 16-256 allowed */
	start += RX_FIFO_SIZE;

	/* HNPTXFSIZ, 0x028 */
	hp->np_fifo = (TX0_FIFO_SIZE << 16) | start;		/* EP0, Tx fifo size */
	start += TX0_FIFO_SIZE;

	hp->in_fifo[0] = (TX1_FIFO_SIZE << 16) | start;		/* EP1, Tx fifo size */
	start += TX1_FIFO_SIZE;
	hp->in_fifo[1] = (TX2_FIFO_SIZE << 16) | start;		/* EP2, Tx fifo size */
	start += TX2_FIFO_SIZE;
	hp->in_fifo[2] = (TX3_FIFO_SIZE << 16) | start;		/* EP3, Tx fifo size */
	// start += TX3_FIFO_SIZE;

	flush_tx_fifo_all ();
	flush_rx_fifo ();

	/* Clear pending interrupts */
	hp->all_int = 0xffffffff;

	/* Mask off all endpoint interrupts */
	hp->in_imask = 0;
	hp->out_imask = 0;
	hp->all_mask = 0;

	for ( i=0; i<NUM_EP; i++ ) {
	    hp->in_ep[i].ctl = 0;
	    hp->in_ep[i].size = 0;
	    hp->in_ep[i].ir = 0xff;

	    hp->out_ep[i].ctl = 0;
	    hp->out_ep[i].size = 0;
	    hp->out_ep[i].ir = 0xff;
	}

	/* Entirely bogus */
	// hp->in_imask = IN_EP_INT_TXU;	/* XXX - enable underrun interrupt */

	enable_usb_ints ();

	// This will be clean
	// dump_fifo_ram ();

	// printf ( "USB Initialization done\n" );

	// show_reg ( "USB is", &hp->is );

	delay ( 10 );
	// printf ( "\n" );

	// dump_fifo_ram ();

	// show_stuff ();
}

/* THE END */
