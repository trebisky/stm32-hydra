/* regs.h
 * Tom Trebisky  4-23-2025
 *
 * This describes the hardware registers for an STMF4xx USB controller
 * This is new code written for Hydra in the style I prefer
 */

struct usb {
		vu32		g_csr;		/* 00 - GOTGTCL - only used in host mode */
		vu32		g_otgint;	/* 04 - GOTGINT - only OTG events */
		vu32		g_ahb_cfg;	/* 08 - GAHBCFG */
		vu32		g_config;	/* 0C = GUSBCFG */
		vu32		g_reset;	/* 10 = GRSTCTL */
		vu32		g_istat;	/* 14 = GINTStatus */
		vu32		g_imask;	/* 18 = GINTMSK */
};

#ifdef notdef
 __IO uint32_t GUSBCFG;      /* Core USB Configuration Register    00Ch*/
  __IO uint32_t GRSTCTL;      /* Core Reset Register                010h*/
  __IO uint32_t GINTStatus;      /* Core Interrupt Register            014h*/
  __IO uint32_t GINTMSK;      /* Core Interrupt Mask Register       018h*/
  __IO uint32_t GRXStatusR;      /* Receive Sts Q Read Register        01Ch*/
  __IO uint32_t GRXStatusP;      /* Receive Sts Q Read & POP Register  020h*/
  __IO uint32_t GRXFSIZ;      /* Receive FIFO Size Register         024h*/
  __IO uint32_t DIEPTXF0_HNPTXFSIZ;   /* EP0 / Non Periodic Tx FIFO Size Register 028h*/
  __IO uint32_t HNPTXStatus;     /* Non Periodic Tx FIFO/Queue Sts reg 02Ch*/
  uint32_t Reserved30[2];     /* Reserved                           030h*/
  __IO uint32_t GCCFG;        /* General Purpose IO Register        038h*/
  __IO uint32_t CID;          /* User ID Register                   03Ch*/
  uint32_t  Reserved40[48];   /* Reserved                      040h-0FFh*/
  __IO uint32_t HPTXFSIZ; /* Host Periodic Tx FIFO Size Reg     100h*/
  __IO uint32_t DIEPTXF[MAX_TX_FIFOS];/* dev Periodic Transmit FIFO */
#endif

/* THE END */
