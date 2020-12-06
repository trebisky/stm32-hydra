/* A bare start to a USB driver.
 *  - for now just placeholder stubs for
 *  entries I have added to locore.s
 * Tom Trebisky  12-6-2020
 */

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
	volatile unsigned int status;		/* 0x918 Tx fifo status */
	    int _pad2;
};

struct usb_hw {
	volatile unsigned int csr;	/* 0x000 GOTGCTL */
	volatile unsigned int ir;	/* 0x004 GOTGINT */
	volatile unsigned int ahb_conf;	/* 0x008 GAHBCFG */
	volatile unsigned int usb_conf;	/* 0x00c GUSBCFG */
	volatile unsigned int reset;	/* 0x010 GRSTCTL */
	volatile unsigned int is;	/* 0x014 GINTSTS - int status */
	volatile unsigned int im;	/* 0x018 GINTMSK - int mask */
	volatile unsigned int rx;	/* 0x01c GRXSTSR */
	volatile unsigned int rxp;	/* 0x020 GRXSTSP */
	volatile unsigned int rx_fifo;	/* 0x024 GRXFSIZ - Rx fifo size*/
	volatile unsigned int np_fifo;	/* 0x028 DIEPTXF0 */
	volatile unsigned int qstat;	/* 0x02C HNPTXSTS */
	    int _pad0[2];
	volatile unsigned int conf;	/* 0x038 GCCCFG - core config*/
	volatile unsigned int cid;	/* 0x03C CID - core ID */
	    int _pad1[64-16];
	volatile unsigned int hp_fifo;	/* 0x100 HNPTXSTS */
	volatile unsigned int in_fifo1;	/* 0x104 DIEPTXFx */
	volatile unsigned int in_fifo2;	/* 0x108 DIEPTXFx */
	volatile unsigned int in_fifo3;	/* 0x10C DIEPTXFx */
	    int _pad2[64*3-4];

	/* Host mode registers.
	 * I don't plan to use this, so I am only
	 * setting up the first one as a place holder.
	 * See the RM for the rest if you ever get
	 * interested.
	 */
	volatile unsigned int hcfg;	/* 0x400 host config */
	    int _pad3[64*4-1];

	volatile unsigned int dcfg;	/* 0x800 device config */
	volatile unsigned int dctl;	/* 0x804 device control */
	volatile unsigned int dstat;	/* 0x808 device status */
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
	struct usb_endpoint in_ep[4];		/* 0x900 */
	    int _pad8[128-4*8];

	/* device - OUT endpoint registers */	/* 0xB00 */
	struct usb_endpoint out_ep[4];
	    int _pad9[128+64-4*8];

	/* power and clock gating */
	volatile unsigned int clock;		/* 0xE00 - power and clock gating */
};

#define IRQ_USB_WAKEUP	42
#define IRQ_USB_FS	67

/* Register base */
#define USB_BASE	(struct usb_hw *) 0x50000000

/* Direct access here only for debugging */
#define USB_RAM_BASE	0x50020000

/* Interrupt handlers.
 */
void usb_wakeup ( void )
{
	printf ( "USB wakeup interrupt" );
}

void usb_fs ( void )
{
	printf ( "USB OTG FS interrupt" );
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
usb_init ( void )
{
	struct usb_hw *hp = USB_BASE;

	show_reg ( "USB csr", &hp->csr );
	show_reg ( "USB cid", &hp->cid );
	show_reg ( "USB hcfg", &hp->hcfg );
	show_reg ( "USB clock", &hp->clock );

	nvic_enable ( IRQ_USB_WAKEUP );
	nvic_enable ( IRQ_USB_FS );
}

/* THE END */
