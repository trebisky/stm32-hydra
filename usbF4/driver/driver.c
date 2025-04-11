/**
  ******************************************************************************
  * @file    usb_core.c
  * @date    22-July-2011
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */

/* driver.c -- was driver/usb_core.c
 */

#include "types.h"
#include "usb_conf.h"

#include "usb_core.h"

#include "usb_regs.h"
#include "usb_defines.h"

/* Moved here from library/core.c  4/6/2025
 */
void
HW_SetFeature ( HANDLE  *pdev, uint8_t test_mode )
{
    DCTL_TypeDef     dctl;

    dctl.d32 = READ_REG32(&pdev->hw->DREGS->DCTL);

    switch (test_mode) {
    case 1: // TEST_J
      dctl.b.tstctl = 1;
      break;
      
    case 2: // TEST_K	
      dctl.b.tstctl = 2;
      break;
      
    case 3: // TEST_SE0_NAK
      dctl.b.tstctl = 3;
      break;
      
    case 4: // TEST_PACKET
      dctl.b.tstctl = 4;
      break;
      
    case 5: // TEST_FORCE_ENABLE
      dctl.b.tstctl = 5;
      break;
    }

    WRITE_REG32(&pdev->hw->DREGS->DCTL, dctl.d32);
}


/**
* @brief  EnableCommonInt
*         Initializes the commmon interrupts, used in both device and modes
* @param  pdev : Selected device
* @retval None
*/
static void
EnableCommonInt(HANDLE *pdev)
{
  GINTMSK_TypeDef  int_mask;
  
  int_mask.d32 = 0;
  /* Clear any pending USB_OTG Interrupts */
#ifndef USE_OTG_MODE
  WRITE_REG32( &pdev->hw->GREGS->GOTGINT, 0xFFFFFFFF);
#endif
  /* Clear any pending interrupts */
  WRITE_REG32( &pdev->hw->GREGS->GINTSTS, 0xFFFFFFFF);

  /* Enable the interrupts in the INTMSK */
  int_mask.b.wkupintr = 1;
  int_mask.b.usbsuspend = 1; 
  
#ifdef USE_OTG_MODE
  int_mask.b.otgintr = 1;
  int_mask.b.sessreqintr = 1;
  int_mask.b.conidstschng = 1;
#endif
  WRITE_REG32( &pdev->hw->GREGS->GINTMSK, int_mask.d32);
}

/**
* @brief  CoreReset : Soft reset of the core
* @param  pdev : Selected device
* @retval STS : status
*/
static STS CoreReset(HANDLE *pdev)
{
  STS status = OK;
  __IO GRSTCTL_TypeDef  greset;
  uint32_t count = 0;
  
  greset.d32 = 0;
  /* Wait for AHB master IDLE state. */
  do {
    // board_uDelay(3);
    delay_us ( 3 );
    greset.d32 = READ_REG32(&pdev->hw->GREGS->GRSTCTL);
    if (++count > 200000)
      return OK;
  } while (greset.b.ahbidle == 0);

  /* Core Soft Reset */
  count = 0;
  greset.b.csftrst = 1;
  WRITE_REG32(&pdev->hw->GREGS->GRSTCTL, greset.d32 );

  do {
    greset.d32 = READ_REG32(&pdev->hw->GREGS->GRSTCTL);
    if (++count > 200000)
      break;
  } while (greset.b.csftrst == 1);

  /* Wait for 3 PHY Clocks*/
  // board_uDelay(3);
  delay_us ( 3 );

  return status;
}

/**
* @brief  WritePacket : Writes a packet into the Tx FIFO associated 
*         with the EP
* @param  pdev : Selected device
* @param  src : source pointer
* @param  ch_ep_num : end point number
* @param  bytes : No. of bytes
* @retval STS : status
*/
STS
WritePacket(HANDLE *pdev, 
                                uint8_t             *src, 
                                uint8_t             ch_ep_num, 
                                uint16_t            len)
{
  STS status = OK;

  usb_debug ( DM_ORIG, "Endpoint %d, write packet %d bytes to FIFO: %c%c%c\n", ch_ep_num, len, src[0], src[1], src[2] );

  if (pdev->cfg.dma_enable == 0) {
    __IO uint32_t *fifo = pdev->hw->DFIFO[ch_ep_num];
    uint32_t count32b =  (len + 3) / 4;

    for (uint32_t i = 0; i < count32b; i++, src+=4) {
      WRITE_REG32( fifo, *((uint32_t *)src) );
    }
  }
  return status;
}

/**
* @brief  ReadPacket : Reads a packet from the Rx FIFO
* @param  pdev : Selected device
* @param  dest : Destination Pointer
* @param  bytes : No. of bytes
*/
void *
ReadPacket(HANDLE *pdev, 
                         uint8_t *dest, 
                         uint16_t len)
{
  uint32_t count32b = (len + 3) / 4;

  // debug moved to calling routine where we know the endpoit.
  // usb_debug ( DM_ORIG, "Endpoint ??, read packet %d bytes from FIFO\n", len );
  
  __IO uint32_t *fifo = pdev->hw->DFIFO[0];
  
  for (uint32_t i = 0; i < count32b; i++, dest += 4 ) {
    *(uint32_t *)dest = READ_REG32(fifo);
  }

  return ((void *)dest);
}

/* tjt */
static struct core_regs core_info;
static int core_count = 0;

/**
* @brief  SelectCore 
*         Initialize core registers address.
* @param  pdev : Selected device
* @param  coreID : USB OTG Core ID
* @retval STS : status
*/
STS
SelectCore(HANDLE *pdev, CORE_ID_TypeDef coreID)
{
  uint32_t i , baseAddress = 0;
  STS status = OK;
  
  pdev->cfg.dma_enable       = 0;
  
  /* at startup the core is in FS mode */
  pdev->cfg.speed            = SPEED_FULL;
  pdev->cfg.mps              = FS_MAX_PACKET_SIZE ;    
  
  /* initialize device cfg following its address */
  if (coreID == FS_CORE_ID) {
	printf ( "Initializing FS core\n" );
    baseAddress                = FS_BASE_ADDR;
    pdev->cfg.coreID           = FS_CORE_ID;
    pdev->cfg.host_channels    = 8 ;
    pdev->cfg.dev_endpoints    = 4 ;
    pdev->cfg.TotalFifoSize    = 320; /* in 32-bits */
    pdev->cfg.phy_itface       = EMBEDDED_PHY;     
    
#ifdef FS_SOF_OUTPUT_ENABLED    
    pdev->cfg.Sof_output       = 1;    
#endif 
    
#ifdef FS_LOW_PWR_MGMT_SUPPORT    
    pdev->cfg.low_power        = 1;    
#endif     
  } else if (coreID == HS_CORE_ID) {
	printf ( "Initializing HS core\n" );
    baseAddress                = HS_BASE_ADDR;
    pdev->cfg.coreID           = HS_CORE_ID;    
    pdev->cfg.host_channels    = 12 ;
    pdev->cfg.dev_endpoints    = 6 ;
    pdev->cfg.TotalFifoSize    = 1280;/* in 32-bits */
    
#ifdef ULPI_PHY_ENABLED
    pdev->cfg.phy_itface       = ULPI_PHY;
#else
 #ifdef EMBEDDED_PHY_ENABLED
    pdev->cfg.phy_itface       = EMBEDDED_PHY;
 #endif
#endif
    
#ifdef HS_INTERNAL_DMA_ENABLED    
    pdev->cfg.dma_enable       = 1;    
#endif
    
#ifdef HS_SOF_OUTPUT_ENABLED    
    pdev->cfg.Sof_output       = 1;    
#endif 
    
#ifdef HS_LOW_PWR_MGMT_SUPPORT    
    pdev->cfg.low_power        = 1;    
#endif 
    
  }

	printf ( "USB base address: %X\n", baseAddress );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
	printf ( "host channels: %d\n", pdev->cfg.host_channels );
	printf ( "pdev: %X\n", pdev );

	if ( core_count > 0 )
		panic ( "Only one core allowed at this time\n" );

	pdev->hw = &core_info;
	printf ( "pdev-hw: %X\n", pdev->hw );
  
  pdev->hw->GREGS = (GREGS *)(baseAddress + \
			CORE_GLOBAL_REGS_OFFSET);

  pdev->hw->DREGS =  (DREGS  *)  (baseAddress + \
			DEV_GLOBAL_REG_OFFSET);
  
  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    pdev->hw->INEP_REGS[i]  = (INEPREGS *)  \
		  (baseAddress + DEV_IN_EP_REG_OFFSET + \
				(i * EP_REG_OFFSET));
    pdev->hw->OUTEP_REGS[i] = (OUTEPREGS *) \
		  (baseAddress + DEV_OUT_EP_REG_OFFSET + \
				(i * EP_REG_OFFSET));
  }

#ifdef USE_HOST_MODE
  pdev->hw->HPRT0 = (uint32_t *)(baseAddress + HOST_PORT_REGS_OFFSET);

  pdev->hw->HREGS = (HREGS *)(baseAddress + \
			HOST_GLOBAL_REG_OFFSET);

  /* 8 (FS)  or 12 (HS) */
  for (i = 0; i < pdev->cfg.host_channels; i++) {
    pdev->hw->HC_REGS[i] = (HC_REGS *)(baseAddress + \
		  HOST_CHAN_REGS_OFFSET + \
				(i * CHAN_REGS_OFFSET));
  }
#endif

  // 4-6-2025 fix bug. We were looping over 12 here,
  // but the DFIFO array was only size 6 (for endpoints).
  // for (i = 0; i < pdev->cfg.host_channels; i++) {
  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    pdev->hw->DFIFO[i] = (uint32_t *)(baseAddress + DATA_FIFO_OFFSET +\
			  (i * DATA_FIFO_SIZE));
  }

  pdev->hw->PCGCCTL = (uint32_t *)(baseAddress + PCGCCTL_OFFSET);

  return status;
}


/**
* @brief  CoreInit
*         Initializes the USB_OTG controller registers and prepares the core
*         device mode or host mode operation.
* @param  pdev : Selected device
* @retval STS : status
*/
STS CoreInit(HANDLE *pdev)
{
  STS status = OK;
  GUSBCFG_TypeDef  usbcfg;
  GCCFG_TypeDef    gccfg;
  GAHBCFG_TypeDef  ahbcfg;
#if defined (STM32F446xx) || defined (STM32F469_479xx)
  DCTL_TypeDef     dctl;
#endif
  usbcfg.d32 = 0;
  gccfg.d32 = 0;
  ahbcfg.d32 = 0;

  if (pdev->cfg.phy_itface == ULPI_PHY)
  {
    gccfg.d32 = READ_REG32(&pdev->hw->GREGS->GCCFG);
    gccfg.b.pwdn = 0;
    
    if (pdev->cfg.Sof_output)
    {
      gccfg.b.sofouten = 1;   
    }
    WRITE_REG32 (&pdev->hw->GREGS->GCCFG, gccfg.d32);
    
    /* Init The ULPI Interface */
    usbcfg.d32 = 0;
    usbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);
    
    usbcfg.b.physel            = 0; /* HS Interface */
#ifdef INTERNAL_VBUS_ENABLED
    usbcfg.b.ulpi_ext_vbus_drv = 0; /* Use internal VBUS */
#else
 #ifdef EXTERNAL_VBUS_ENABLED
    usbcfg.b.ulpi_ext_vbus_drv = 1; /* Use external VBUS */
 #endif
#endif
    usbcfg.b.term_sel_dl_pulse = 0; /* Data line pulsing using utmi_txvalid */    
    
    usbcfg.b.ulpi_fsls = 0;
    usbcfg.b.ulpi_clk_sus_m = 0;
    WRITE_REG32 (&pdev->hw->GREGS->GUSBCFG, usbcfg.d32);
    
    /* Reset after a PHY select  */
    CoreReset(pdev);
    
    if(pdev->cfg.dma_enable == 1) {
      
      ahbcfg.b.hburstlen = 5; /* 64 x 32-bits*/
      ahbcfg.b.dmaenable = 1;
      WRITE_REG32(&pdev->hw->GREGS->GAHBCFG, ahbcfg.d32);
      
    }
  } else { /* FS interface (embedded Phy) */
    
    usbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);;
    usbcfg.b.physel  = 1; /* FS Interface */
    WRITE_REG32 (&pdev->hw->GREGS->GUSBCFG, usbcfg.d32);
    /* Reset after a PHY select and set Host mode */
    CoreReset(pdev);
    /* Deactivate the power down*/
    gccfg.d32 = 0;
    gccfg.b.pwdn = 1;
    gccfg.b.vbussensingA = 1;
    gccfg.b.vbussensingB = 1;
   
#ifndef VBUS_SENSING_ENABLED
    gccfg.b.disablevbussensing = 1;
#endif
    
    if(pdev->cfg.Sof_output)
      gccfg.b.sofouten = 1;  
    
    WRITE_REG32 (&pdev->hw->GREGS->GCCFG, gccfg.d32);
    // board_mDelay(20);
	delay_ms ( 20 );
  }

  /* case the HS core is working in FS mode */
  if(pdev->cfg.dma_enable == 1) {
    
    ahbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GAHBCFG);
    ahbcfg.b.hburstlen = 5; /* 64 x 32-bits*/
    ahbcfg.b.dmaenable = 1;
    WRITE_REG32(&pdev->hw->GREGS->GAHBCFG, ahbcfg.d32);
    
  }

  /* initialize OTG features */
#ifdef  USE_OTG_MODE
  usbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);
  usbcfg.b.hnpcap = 1;
  usbcfg.b.srpcap = 1;
  WRITE_REG32(&pdev->hw->GREGS->GUSBCFG, usbcfg.d32);
  EnableCommonInt(pdev);
#endif
  
/* XXX - who knows what this is all about */
#if defined (STM32F446xx) || defined (STM32F469_479xx)
  usbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);
  usbcfg.b.srpcap = 1;

  /* XXX - this is the special part */
  /*clear sdis bit in dctl */
  dctl.d32 = READ_REG32(&pdev->hw->DREGS->DCTL);
  /* Connect device */
  dctl.b.sftdiscon  = 0;
  WRITE_REG32(&pdev->hw->DREGS->DCTL, dctl.d32);
  dctl.d32 = READ_REG32(&pdev->hw->DREGS->DCTL);

  WRITE_REG32(&pdev->hw->GREGS->GUSBCFG, usbcfg.d32);
  EnableCommonInt(pdev);
#endif
  
  return status;
}

/**
* @brief  EnableGlobalInt
*         Enables the controller's Global Int in the AHB Config reg
*
* Note the order of arguments to MODIFY_REG32
*/
void
EnableGlobalInt (HANDLE *pdev)
{
  // STS status = OK;
  GAHBCFG_TypeDef  val;
  
  val.d32 = 0;
  val.b.glblintrmsk = 1; /* Enable interrupts */
  MODIFY_REG32(&pdev->hw->GREGS->GAHBCFG, 0, val.d32);
  // return status;
}


/**
* @brief  DisableGlobalInt
*         Enables the controller's Global Int in the AHB Config reg
*
* Note the order of arguments to MODIFY_REG32
*/
void
DisableGlobalInt(HANDLE *pdev)
{
  // STS status = OK;

  GAHBCFG_TypeDef  val;
  val.d32 = 0;
  val.b.glblintrmsk = 1; /* Enable interrupts */

  MODIFY_REG32(&pdev->hw->GREGS->GAHBCFG, val.d32, 0);
  // return status;
}

/**
* @brief  FlushTxFifo : Flush a Tx FIFO
* @param  pdev : Selected device
* @param  num : FO num
* @retval STS : status
*/
STS FlushTxFifo (HANDLE *pdev , uint32_t num )
{
  STS status = OK;
  __IO GRSTCTL_TypeDef  greset;
  
  uint32_t count = 0;

  usb_debug ( DM_ORIG, "USB - flush Tx Fifo %d\n", num ); 

  greset.d32 = 0;
  greset.b.txfflsh = 1;
  greset.b.txfnum  = num;
  WRITE_REG32( &pdev->hw->GREGS->GRSTCTL, greset.d32 );

  do {
    greset.d32 = READ_REG32( &pdev->hw->GREGS->GRSTCTL);
    if (++count > 200000) {
      break;
    }
  } while (greset.b.txfflsh == 1);

  /* Wait for 3 PHY Clocks*/
  // board_uDelay(3);
  delay_us ( 3 );

  return status;
}


/**
* @brief  FlushRxFifo : Flush a Rx FIFO
* @param  pdev : Selected device
* @retval STS : status
*/
STS FlushRxFifo( HANDLE *pdev )
{
  STS status = OK;
  __IO GRSTCTL_TypeDef  greset;
  uint32_t count = 0;

  usb_debug ( DM_ORIG, "USB - flush Rx Fifo\n" ); 
  
  greset.d32 = 0;
  greset.b.rxfflsh = 1;
  WRITE_REG32( &pdev->hw->GREGS->GRSTCTL, greset.d32 );

  do {
    greset.d32 = READ_REG32( &pdev->hw->GREGS->GRSTCTL);
    if (++count > 200000) {
      break;
    }
  } while (greset.b.rxfflsh == 1);

  /* Wait for 3 PHY Clocks*/
  // board_uDelay(3);
  delay_us ( 3 );

  return status;
}


/**
* @brief  SetCurrentMode : Set ID line
* @param  pdev : Selected device
* @param  mode :  (Host/device)
* @retval STS : status
*/
STS
SetCurrentMode(HANDLE *pdev , uint8_t mode)
{
  STS status = OK;
  GUSBCFG_TypeDef  usbcfg;
  
  usbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);
  
  usbcfg.b.force_host = 0;
  usbcfg.b.force_dev = 0;
  
  if ( mode == HOST_MODE) {
    usbcfg.b.force_host = 1;
  } else if ( mode == DEVICE_MODE) {
    usbcfg.b.force_dev = 1;
  }
  
  WRITE_REG32(&pdev->hw->GREGS->GUSBCFG, usbcfg.d32);
  // board_mDelay(50);
  delay_ms ( 50 );

  return status;
}


/**
* @brief  GetMode : Get current mode
* @param  pdev : Selected device
* @retval current mode
*/
uint32_t GetMode(HANDLE *pdev)
{
  return (READ_REG32(&pdev->hw->GREGS->GINTSTS ) & 0x1);
}


/**
* @brief  IsDeviceMode : Check if it is device mode
* @param  pdev : Selected device
* @retval num_in_ep
*/
uint8_t IsDeviceMode(HANDLE *pdev)
{
  return (GetMode(pdev) != HOST_MODE);
}


/**
* @brief  IsHostMode : Check if it is host mode
* @param  pdev : Selected device
* @retval num_in_ep
*/
uint8_t IsHostMode(HANDLE *pdev)
{
  return (GetMode(pdev) == HOST_MODE);
}


/**
* @brief  ReadCoreItr : returns the Core Interrupt register
* @param  pdev : Selected device
* @retval Status
*/
uint32_t ReadCoreItr(HANDLE *pdev)
{
  uint32_t v = 0;
  v = READ_REG32(&pdev->hw->GREGS->GINTSTS);
  v &= READ_REG32(&pdev->hw->GREGS->GINTMSK);
  return v;
}


/**
* @brief  ReadOtgItr : returns the USB_OTG Interrupt register
* @param  pdev : Selected device
* @retval Status
*/
uint32_t ReadOtgItr (HANDLE *pdev)
{
  return (READ_REG32 (&pdev->hw->GREGS->GOTGINT));
}

#ifdef USE_DEVICE_MODE
/*         PCD Core Layer       */

/**
* @brief  InitDevSpeed :Initializes the DevSpd field of DCFG register 
*         depending the PHY type and the enumeration speed of the device.
* @param  pdev : Selected device
* @retval : None
*/
void InitDevSpeed(HANDLE *pdev , uint8_t speed)
{
  DCFG_TypeDef   dcfg;
  
  dcfg.d32 = READ_REG32(&pdev->hw->DREGS->DCFG);
  dcfg.b.devspd = speed;
  WRITE_REG32(&pdev->hw->DREGS->DCFG, dcfg.d32);
}


/**
* @brief  CoreInitDev : Initializes the USB_OTG controller registers 
*         for device mode
* @param  pdev : Selected device
* @retval STS : status
*/
STS CoreInitDev (HANDLE *pdev)
{
  STS             status       = OK;
  DEPCTL_TypeDef  depctl;
  uint32_t i;
  DCFG_TypeDef    dcfg;
  FSIZ_TypeDef    nptxfifosize;
  FSIZ_TypeDef    txfifosize;
  DIEPMSK_TypeDef msk;
  DTHRCTL_TypeDef dthrctl;  

  depctl.d32 = 0;
  dcfg.d32 = 0;
  nptxfifosize.d32 = 0;
  txfifosize.d32 = 0;
  msk.d32 = 0;
  
  /* Restart the Phy Clock */
  WRITE_REG32(pdev->hw->PCGCCTL, 0);
  /* Device configuration register */
  dcfg.d32 = READ_REG32( &pdev->hw->DREGS->DCFG);
  dcfg.b.perfrint = DCFG_FRAME_INTERVAL_80;
  WRITE_REG32( &pdev->hw->DREGS->DCFG, dcfg.d32 );

#ifdef FS_CORE
  if(pdev->cfg.coreID == FS_CORE_ID  ) {  
    /* Set Full speed phy */
    InitDevSpeed (pdev , SPEED_PARAM_FULL);
    
    /* set Rx FIFO size */
    WRITE_REG32(&pdev->hw->GREGS->GRXFSIZ, RX_FIFO_FS_SIZE);
    
    /* EP0 TX*/
    nptxfifosize.b.depth     = TX0_FIFO_FS_SIZE;
    nptxfifosize.b.startaddr = RX_FIFO_FS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF0_HNPTXFSIZ, nptxfifosize.d32 );
    
    /* EP1 TX*/
    txfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
    txfifosize.b.depth = TX1_FIFO_FS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[0], txfifosize.d32 );
    
    /* EP2 TX*/
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX2_FIFO_FS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[1], txfifosize.d32 );
    
    /* EP3 TX*/  
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX3_FIFO_FS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[2], txfifosize.d32 );
  }
#endif

#ifdef HS_CORE
  if(pdev->cfg.coreID == HS_CORE_ID  ) {

    /* Set High speed phy */
    if(pdev->cfg.phy_itface  == ULPI_PHY) {
      InitDevSpeed (pdev , SPEED_PARAM_HIGH);
    } else { /* set High speed phy in Full speed mode */
      InitDevSpeed (pdev , SPEED_PARAM_HIGH_IN_FULL);
    }
    
    /* set Rx FIFO size */
    WRITE_REG32(&pdev->hw->GREGS->GRXFSIZ, RX_FIFO_HS_SIZE);
    
    /* EP0 TX*/
    nptxfifosize.b.depth     = TX0_FIFO_HS_SIZE;
    nptxfifosize.b.startaddr = RX_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF0_HNPTXFSIZ, nptxfifosize.d32 );
    
    /* EP1 TX*/
    txfifosize.b.startaddr = nptxfifosize.b.startaddr + nptxfifosize.b.depth;
    txfifosize.b.depth = TX1_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[0], txfifosize.d32 );
    
    /* EP2 TX*/
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX2_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[1], txfifosize.d32 );
    
    /* EP3 TX*/  
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX3_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[2], txfifosize.d32 );
    
    /* EP4 TX*/
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX4_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[3], txfifosize.d32 );
    
    /* EP5 TX*/  
    txfifosize.b.startaddr += txfifosize.b.depth;
    txfifosize.b.depth = TX5_FIFO_HS_SIZE;
    WRITE_REG32( &pdev->hw->GREGS->DIEPTXF[4], txfifosize.d32 );
  }
#endif  

  /* Flush the FIFOs */
  FlushTxFifo(pdev , 0x10); /* all Tx FIFOs */
  FlushRxFifo(pdev);

  /* Clear all pending Device Interrupts */
  WRITE_REG32( &pdev->hw->DREGS->DIEPMSK, 0 );
  WRITE_REG32( &pdev->hw->DREGS->DOEPMSK, 0 );
  WRITE_REG32( &pdev->hw->DREGS->DAINT, 0xFFFFFFFF );
  WRITE_REG32( &pdev->hw->DREGS->DAINTMSK, 0 );


  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    depctl.d32 = READ_REG32(&pdev->hw->INEP_REGS[i]->DIEPCTL);
    if (depctl.b.epena) {
      depctl.d32 = 0;
      depctl.b.epdis = 1;
      depctl.b.snak = 1;
    } else {
      depctl.d32 = 0;
    }
    WRITE_REG32( &pdev->hw->INEP_REGS[i]->DIEPCTL, depctl.d32);
    WRITE_REG32( &pdev->hw->INEP_REGS[i]->DIEPTSIZ, 0);
    WRITE_REG32( &pdev->hw->INEP_REGS[i]->DIEPINT, 0xFF);
  }

  for (i = 0; i <  pdev->cfg.dev_endpoints; i++) {
    DEPCTL_TypeDef  depctl;
    depctl.d32 = READ_REG32(&pdev->hw->OUTEP_REGS[i]->DOEPCTL);
    if (depctl.b.epena) {
      depctl.d32 = 0;
      depctl.b.epdis = 1;
      depctl.b.snak = 1;
    } else {
      depctl.d32 = 0;
    }
    WRITE_REG32( &pdev->hw->OUTEP_REGS[i]->DOEPCTL, depctl.d32);
    WRITE_REG32( &pdev->hw->OUTEP_REGS[i]->DOEPTSIZ, 0);
    WRITE_REG32( &pdev->hw->OUTEP_REGS[i]->DOEPINT, 0xFF);
  }

  msk.d32 = 0;
  msk.b.txfifoundrn = 1;
  MODIFY_REG32(&pdev->hw->DREGS->DIEPMSK, msk.d32, msk.d32);
  
  if (pdev->cfg.dma_enable == 1) {
    dthrctl.d32 = 0;
    dthrctl.b.non_iso_thr_en = 1;
    dthrctl.b.iso_thr_en = 1;
    dthrctl.b.tx_thr_len = 64;
    dthrctl.b.rx_thr_en = 1;
    dthrctl.b.rx_thr_len = 64;
    WRITE_REG32(&pdev->hw->DREGS->DTHRCTL, dthrctl.d32);  
  }

  EnableDevInt(pdev);

  return status;
}

/**
* @brief  EnableDevInt : Enables the Device mode interrupts
* @param  pdev : Selected device
* @retval STS : status
*/
STS EnableDevInt(HANDLE *pdev)
{
  STS status = OK;
  GINTMSK_TypeDef  intmsk;
  
  intmsk.d32 = 0;
  
  /* Disable all interrupts. */
  WRITE_REG32( &pdev->hw->GREGS->GINTMSK, 0);
  /* Clear any pending interrupts */
  WRITE_REG32( &pdev->hw->GREGS->GINTSTS, 0xFFFFFFFF);
  /* Enable the common interrupts */
  EnableCommonInt(pdev);
  
  if (pdev->cfg.dma_enable == 0) {
    intmsk.b.rxstsqlvl = 1;
  }
  
  /* Enable interrupts matching to the Device mode ONLY */
  intmsk.b.usbsuspend = 1;
  intmsk.b.usbreset   = 1;
  intmsk.b.enumdone   = 1;
  intmsk.b.inepintr   = 1;
  intmsk.b.outepintr  = 1;
  intmsk.b.sofintr    = 1; 
  
  intmsk.b.incomplisoin    = 1; 
  intmsk.b.incomplisoout    = 1;   
#ifdef VBUS_SENSING_ENABLED
  intmsk.b.sessreqintr    = 1; 
  intmsk.b.otgintr    = 1;    
#endif  
  MODIFY_REG32( &pdev->hw->GREGS->GINTMSK, intmsk.d32, intmsk.d32);
  return status;
}

/**
* Called when speed enumeration is done.
* @brief  GetDeviceSpeed
*         Get the device speed from the device status register
* @param  None
* @retval status
*/
enum SPEED
GetDeviceSpeed (HANDLE *pdev)
{
  DSTS_TypeDef  dsts;
  enum SPEED speed = USB_SPEED_UNKNOWN;
  
  
  dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
  
  switch (dsts.b.enumspd)
  {
  case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
    speed = USB_SPEED_HIGH;
    break;

  case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
  case DSTS_ENUMSPD_FS_PHY_48MHZ:
    speed = USB_SPEED_FULL;
    break;
    
  case DSTS_ENUMSPD_LS_PHY_6MHZ:
    speed = USB_SPEED_LOW;
    break;

  default:
    speed = USB_SPEED_FULL;
    break; 
  }
  
  return speed;
}

/**
* @brief  enables EP0 OUT to receive SETUP packets and configures EP0
*   for transmitting packets
* @param  None
* @retval STS : status
*/
STS  EP0Activate(HANDLE *pdev)
{
  STS             status = OK;
  DSTS_TypeDef    dsts;
  DEPCTL_TypeDef  diepctl;
  DCTL_TypeDef    dctl;
  
  dctl.d32 = 0;
  /* Read the Device Status and Endpoint 0 Control registers */
  dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
  diepctl.d32 = READ_REG32(&pdev->hw->INEP_REGS[0]->DIEPCTL);
  /* Set the MPS of the IN EP based on the enumeration speed */
  switch (dsts.b.enumspd)
  {
  case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
  case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
  case DSTS_ENUMSPD_FS_PHY_48MHZ:
    diepctl.b.mps = DEP0CTL_MPS_64;
    break;
  case DSTS_ENUMSPD_LS_PHY_6MHZ:
    diepctl.b.mps = DEP0CTL_MPS_8;
    break;
  default:
    diepctl.b.mps = DEP0CTL_MPS_64;
    break; 
  }
  WRITE_REG32(&pdev->hw->INEP_REGS[0]->DIEPCTL, diepctl.d32);
  dctl.b.cgnpinnak = 1;
  MODIFY_REG32(&pdev->hw->DREGS->DCTL, dctl.d32, dctl.d32);
  return status;
}


/**
* @brief  EPActivate : Activates an EP
* @param  pdev : Selected device
* @retval STS : status
*/
STS EPActivate(HANDLE *pdev , EP *ep)
{
  STS status = OK;
  DEPCTL_TypeDef  depctl;
  DAINT_TypeDef  daintmsk;
  __IO uint32_t *addr;
  
  
  depctl.d32 = 0;
  daintmsk.d32 = 0;
  /* Read DEPCTLn register */
  if (ep->is_in == 1) {
    addr = &pdev->hw->INEP_REGS[ep->num]->DIEPCTL;
    daintmsk.ep.in = 1 << ep->num;
  } else {
    addr = &pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL;
    daintmsk.ep.out = 1 << ep->num;
  }

  /* If the EP is already active don't change the EP Control
  * register. */
  depctl.d32 = READ_REG32(addr);
  if (!depctl.b.usbactep) {
    depctl.b.mps    = ep->maxpacket;
    depctl.b.eptype = ep->type;
    depctl.b.txfnum = ep->tx_fifo_num;
    depctl.b.setd0pid = 1;
    depctl.b.usbactep = 1;
    WRITE_REG32(addr, depctl.d32);
  }

  /* Enable the Interrupt for this EP */
#ifdef HS_DEDICATED_EP1_ENABLED
  if((ep->num == 1)&&(pdev->cfg.coreID == HS_CORE_ID)) {
    MODIFY_REG32(&pdev->hw->DREGS->DEACHMSK, 0, daintmsk.d32);
  } else
#endif   
    MODIFY_REG32(&pdev->hw->DREGS->DAINTMSK, 0, daintmsk.d32);

  return status;
}


/**
* @brief  EPDeactivate : Deactivates an EP
* @param  pdev : Selected device
* @retval STS : status
*/
STS EPDeactivate(HANDLE *pdev , EP *ep)
{
  STS status = OK;
  DEPCTL_TypeDef  depctl;
  DAINT_TypeDef  daintmsk;
  __IO uint32_t *addr;
  
  depctl.d32 = 0;
  daintmsk.d32 = 0;  
  /* Read DEPCTLn register */
  if (ep->is_in == 1) {
    addr = &pdev->hw->INEP_REGS[ep->num]->DIEPCTL;
    daintmsk.ep.in = 1 << ep->num;
  } else {
    addr = &pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL;
    daintmsk.ep.out = 1 << ep->num;
  }

  depctl.b.usbactep = 0;
  WRITE_REG32(addr, depctl.d32);
  /* Disable the Interrupt for this EP */
  
#ifdef HS_DEDICATED_EP1_ENABLED
  if((ep->num == 1)&&(pdev->cfg.coreID == HS_CORE_ID)) {
    MODIFY_REG32(&pdev->hw->DREGS->DEACHMSK, daintmsk.d32, 0);
  } else
#endif    
    MODIFY_REG32(&pdev->hw->DREGS->DAINTMSK, daintmsk.d32, 0);

  return status;
}


/**
* @brief  EPStartXfer : Handle the setup for data xfer for an EP and 
*         starts the xfer
* @param  pdev : Selected device
* @retval STS : status
*/
STS EPStartXfer(HANDLE *pdev , EP *ep)
{
  STS status = OK;
  DEPCTL_TypeDef     depctl;
  DEPXFRSIZ_TypeDef  deptsiz;
  DSTS_TypeDef       dsts;    
  uint32_t fifoemptymsk = 0;  
  
  depctl.d32 = 0;
  deptsiz.d32 = 0;

  usb_debug ( DM_ORIG, "- EP %d StartXfer %d bytes\n", ep->num, ep->xfer_len );

  /* IN endpoint */
  if (ep->is_in == 1) {
    depctl.d32  = READ_REG32(&(pdev->hw->INEP_REGS[ep->num]->DIEPCTL));
    deptsiz.d32 = READ_REG32(&(pdev->hw->INEP_REGS[ep->num]->DIEPTSIZ));

    /* Zero Length Packet? */
    if (ep->xfer_len == 0) {
      deptsiz.b.xfersize = 0;
      deptsiz.b.pktcnt = 1;
    } else {
      /* Program the transfer size and packet count
      * as follows: xfersize = N * maxpacket +
      * short_packet pktcnt = N + (short_packet
      * exist ? 1 : 0)
      */
      deptsiz.b.xfersize = ep->xfer_len;
      deptsiz.b.pktcnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;

      if (ep->type == EP_TYPE_ISOC) {
        deptsiz.b.mc = 1;
      }       
    }
    WRITE_REG32(&pdev->hw->INEP_REGS[ep->num]->DIEPTSIZ, deptsiz.d32);
    
    if (pdev->cfg.dma_enable == 1) {
      WRITE_REG32(&pdev->hw->INEP_REGS[ep->num]->DIEPDMA, ep->dma_addr);
    } else {
      if (ep->type != EP_TYPE_ISOC) {
        /* Enable the Tx FIFO Empty Interrupt for this EP */
        if (ep->xfer_len > 0) {
          fifoemptymsk = 1 << ep->num;
          MODIFY_REG32(&pdev->hw->DREGS->DIEPEMPMSK, 0, fifoemptymsk);
	  usb_debug ( DM_ORIG, "- TxE interrupt enabled for endpoint %d\n", ep->num );
        }
      }
    }
    
    
    if (ep->type == EP_TYPE_ISOC) {
      dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
      
      if (((dsts.b.soffn)&0x1) == 0) {
        depctl.b.setd1pid = 1;
      } else {
        depctl.b.setd0pid = 1;
      }
    } 
    
    /* EP enable, IN data in FIFO */
    depctl.b.cnak = 1;
    depctl.b.epena = 1;
    WRITE_REG32(&pdev->hw->INEP_REGS[ep->num]->DIEPCTL, depctl.d32);
    
    if (ep->type == EP_TYPE_ISOC) {
      WritePacket(pdev, ep->xfer_buff, ep->num, ep->xfer_len);   
    }    

  } else {

    /* OUT endpoint */
    depctl.d32  = READ_REG32(&(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL));
    deptsiz.d32 = READ_REG32(&(pdev->hw->OUTEP_REGS[ep->num]->DOEPTSIZ));
    /* Program the transfer size and packet count as follows:
    * pktcnt = N
    * xfersize = N * maxpacket
    */
    if (ep->xfer_len == 0) {
      deptsiz.b.xfersize = ep->maxpacket;
      deptsiz.b.pktcnt = 1;
    } else {
      deptsiz.b.pktcnt = (ep->xfer_len + (ep->maxpacket - 1)) / ep->maxpacket;
      deptsiz.b.xfersize = deptsiz.b.pktcnt * ep->maxpacket;
      ep->xfer_len = deptsiz.b.xfersize ;
    }

    WRITE_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPTSIZ, deptsiz.d32);
    
    if (pdev->cfg.dma_enable == 1) {
      WRITE_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPDMA, ep->dma_addr);
    }
    
    if (ep->type == EP_TYPE_ISOC) {
      if (ep->even_odd_frame) {
        depctl.b.setd1pid = 1;
      } else {
        depctl.b.setd0pid = 1;
      }
    }

    /* EP enable */
    depctl.b.cnak = 1;
    depctl.b.epena = 1;
    WRITE_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL, depctl.d32);
  }
  return status;
}


/**
* @brief  EP0StartXfer : Handle the setup for a data xfer for EP0 and 
*         starts the xfer
* @param  pdev : Selected device
* @retval STS : status
*/
STS EP0StartXfer(HANDLE *pdev , EP *ep)
{
  STS                 status = OK;
  DEPCTL_TypeDef      depctl;
  DEP0XFRSIZ_TypeDef  deptsiz;
  INEPREGS          *in_regs;
  uint32_t fifoemptymsk = 0;
  
  depctl.d32   = 0;
  deptsiz.d32  = 0;

  usb_debug ( DM_ORIG, "- EP 0 StartXfer %d bytes\n", ep->xfer_len );

  /* IN endpoint */
  if (ep->is_in == 1) {
    in_regs = pdev->hw->INEP_REGS[0];
    depctl.d32  = READ_REG32(&in_regs->DIEPCTL);
    deptsiz.d32 = READ_REG32(&in_regs->DIEPTSIZ);

    /* Zero Length Packet? */
    if (ep->xfer_len == 0) {
      deptsiz.b.xfersize = 0;
      deptsiz.b.pktcnt = 1;
      
    } else {
      if (ep->xfer_len > ep->maxpacket) {
        ep->xfer_len = ep->maxpacket;
        deptsiz.b.xfersize = ep->maxpacket;
      } else {
        deptsiz.b.xfersize = ep->xfer_len;
      }
      deptsiz.b.pktcnt = 1;
    }
    WRITE_REG32(&in_regs->DIEPTSIZ, deptsiz.d32);
    
    if (pdev->cfg.dma_enable == 1) {
      WRITE_REG32(&pdev->hw->INEP_REGS[ep->num]->DIEPDMA, ep->dma_addr);  
    }
    
    /* EP enable, IN data in FIFO */
    depctl.b.cnak = 1;
    depctl.b.epena = 1;
    WRITE_REG32(&in_regs->DIEPCTL, depctl.d32);
    
    
    if (pdev->cfg.dma_enable == 0) {
      /* Enable the Tx FIFO Empty Interrupt for this EP */
      if (ep->xfer_len > 0)
      {
        {
          fifoemptymsk |= 1 << ep->num;
          MODIFY_REG32(&pdev->hw->DREGS->DIEPEMPMSK, 0, fifoemptymsk);
	  usb_debug ( DM_ORIG, "- TxE interrupt enabled for endpoint 0\n" );
        }
      }
    }
  }
  else
  {
    /* OUT endpoint */
    depctl.d32  = READ_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL);
    deptsiz.d32 = READ_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPTSIZ);
    /* Program the transfer size and packet count as follows:
    * xfersize = N * (maxpacket + 4 - (maxpacket % 4))
    * pktcnt = N           */
    if (ep->xfer_len == 0)
    {
      deptsiz.b.xfersize = ep->maxpacket;
      deptsiz.b.pktcnt = 1;
    }
    else
    {
      ep->xfer_len = ep->maxpacket;
      deptsiz.b.xfersize = ep->maxpacket;
      deptsiz.b.pktcnt = 1;
    }
    WRITE_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPTSIZ, deptsiz.d32);
    if (pdev->cfg.dma_enable == 1)
    {
      WRITE_REG32(&pdev->hw->OUTEP_REGS[ep->num]->DOEPDMA, ep->dma_addr);
    }
    /* EP enable */
    depctl.b.cnak = 1;
    depctl.b.epena = 1;
    WRITE_REG32 (&(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL), depctl.d32);
    
  }
  return status;
}

/**
* @brief  EPSetStall : Set the EP STALL
* @param  pdev : Selected device
* @retval STS : status
*/
STS EPSetStall(HANDLE *pdev , EP *ep)
{
  STS status = OK;
  DEPCTL_TypeDef  depctl;
  __IO uint32_t *depctl_addr;
  
  depctl.d32 = 0;
  if (ep->is_in == 1) {
    depctl_addr = &(pdev->hw->INEP_REGS[ep->num]->DIEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);
    /* set the disable and stall bits */
    if (depctl.b.epena) {
      depctl.b.epdis = 1;
    }
    depctl.b.stall = 1;
    WRITE_REG32(depctl_addr, depctl.d32);
  } else {
    depctl_addr = &(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);
    /* set the stall bit */
    depctl.b.stall = 1;
    WRITE_REG32(depctl_addr, depctl.d32);
  }
  return status;
}

/**
* @brief  Clear the EP STALL
* @param  pdev : Selected device
* @retval STS : status
*/
STS
EPClearStall(HANDLE *pdev , EP *ep)
{
  STS status = OK;
  DEPCTL_TypeDef  depctl;
  __IO uint32_t *depctl_addr;
  
  depctl.d32 = 0;
  
  if (ep->is_in == 1) {
    depctl_addr = &(pdev->hw->INEP_REGS[ep->num]->DIEPCTL);
  } else {
    depctl_addr = &(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL);
  }

  depctl.d32 = READ_REG32(depctl_addr);
  /* clear the stall bits */
  depctl.b.stall = 0;
  if (ep->type == EP_TYPE_INTR || ep->type == EP_TYPE_BULK) {
    depctl.b.setd0pid = 1; /* DATA0 */
  }

  WRITE_REG32(depctl_addr, depctl.d32);
  return status;
}

/**
* @brief  ReadDevAllOutEp_itr : returns OUT endpoint interrupt bits
* @param  pdev : Selected device
* @retval OUT endpoint interrupt bits
*/
uint32_t
ReadDevAllOutEp_itr(HANDLE *pdev)
{
  uint32_t v;
  v  = READ_REG32(&pdev->hw->DREGS->DAINT);
  v &= READ_REG32(&pdev->hw->DREGS->DAINTMSK);
  return ((v & 0xffff0000) >> 16);
}

/**
* @brief  ReadDevOutEP_itr : returns Device OUT EP Interrupt register
* @param  pdev : Selected device
* @param  ep : end point number
* @retval Device OUT EP Interrupt register
*/
uint32_t ReadDevOutEP_itr(HANDLE *pdev , uint8_t epnum)
{
  uint32_t v;
  v  = READ_REG32(&pdev->hw->OUTEP_REGS[epnum]->DOEPINT);
  v &= READ_REG32(&pdev->hw->DREGS->DOEPMSK);
  return v;
}

/**
* @brief  ReadDevAllInEPItr : Get int status register
* @param  pdev : Selected device
* @retval int status register
*/
uint32_t ReadDevAllInEPItr(HANDLE *pdev)
{
  uint32_t v;
  v = READ_REG32(&pdev->hw->DREGS->DAINT);
  v &= READ_REG32(&pdev->hw->DREGS->DAINTMSK);
  return (v & 0xffff);
}

/**
* @brief  configures EPO to receive SETUP packets
* @param  None
* @retval : None
*/
void EP0_OutStart(HANDLE *pdev)
{
  DEP0XFRSIZ_TypeDef  doeptsize0;

  doeptsize0.d32 = 0;
  doeptsize0.b.supcnt = 3;
  doeptsize0.b.pktcnt = 1;
  doeptsize0.b.xfersize = 8 * 3;
  WRITE_REG32( &pdev->hw->OUTEP_REGS[0]->DOEPTSIZ, doeptsize0.d32 );
  
  if (pdev->cfg.dma_enable == 1) {
    DEPCTL_TypeDef  doepctl;
    doepctl.d32 = 0;
    WRITE_REG32( &pdev->hw->OUTEP_REGS[0]->DOEPDMA, 
                        (uint32_t)&pdev->dev.setup_packet);
    
    /* EP enable */
    doepctl.d32 = READ_REG32(&pdev->hw->OUTEP_REGS[0]->DOEPCTL);
    doepctl.b.epena = 1;
    doepctl.d32 = 0x80008000;
    WRITE_REG32( &pdev->hw->OUTEP_REGS[0]->DOEPCTL, doepctl.d32);
  }
}

/**
* @brief  RemoteWakeup : active remote wakeup signalling
* @param  None
* @retval : None
*/
void
ActiveRemoteWakeup(HANDLE *pdev)
{
  DCTL_TypeDef     dctl;
  DSTS_TypeDef     dsts;
  PCGCCTL_TypeDef  power;  
  
  if (pdev->dev.DevRemoteWakeup) {
    dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
    if(dsts.b.suspsts == 1) {
      if(pdev->cfg.low_power) {
        /* un-gate USB Core clock */
        power.d32 = READ_REG32(pdev->hw->PCGCCTL);
        power.b.gatehclk = 0;
        power.b.stoppclk = 0;
        WRITE_REG32(pdev->hw->PCGCCTL, power.d32);
      }   
      /* active Remote wakeup signaling */
      dctl.d32 = 0;
      dctl.b.rmtwkupsig = 1;
      MODIFY_REG32(&pdev->hw->DREGS->DCTL, 0, dctl.d32);
      // board_mDelay(5);
	  delay_ms ( 5 );
      MODIFY_REG32(&pdev->hw->DREGS->DCTL, dctl.d32, 0 );
    }
  }
}


/**
* @brief  UngateClock : active USB Core clock
* @param  None
* @retval : None
*/
void UngateClock(HANDLE *pdev)
{
  if(pdev->cfg.low_power)
  {
    
    DSTS_TypeDef     dsts;
    PCGCCTL_TypeDef  power; 
    
    dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
    
    if(dsts.b.suspsts == 1) {
      /* un-gate USB Core clock */
      power.d32 = READ_REG32(pdev->hw->PCGCCTL);
      power.b.gatehclk = 0;
      power.b.stoppclk = 0;
      WRITE_REG32(pdev->hw->PCGCCTL, power.d32);
    }
  }
}

/**
* @brief  Stop the device and clean up fifo's
* @param  None
* @retval : None
*/
void StopDevice(HANDLE *pdev)
{
  uint32_t i;
  
  pdev->dev.device_status = 1;
  
  for (i = 0; i < pdev->cfg.dev_endpoints ; i++) {
    WRITE_REG32( &pdev->hw->INEP_REGS[i]->DIEPINT, 0xFF);
    WRITE_REG32( &pdev->hw->OUTEP_REGS[i]->DOEPINT, 0xFF);
  }
  
  WRITE_REG32( &pdev->hw->DREGS->DIEPMSK, 0 );
  WRITE_REG32( &pdev->hw->DREGS->DOEPMSK, 0 );
  WRITE_REG32( &pdev->hw->DREGS->DAINTMSK, 0 );
  WRITE_REG32( &pdev->hw->DREGS->DAINT, 0xFFFFFFFF );  
  
  /* Flush the FIFO */
  FlushRxFifo(pdev);
  FlushTxFifo(pdev ,  0x10 );  
}

/**
* @brief  returns the EP Status
* @param  pdev : Selected device
*         ep : endpoint structure
* @retval : EP status
*/

uint32_t GetEPStatus(HANDLE *pdev ,EP *ep)
{
  DEPCTL_TypeDef  depctl;
  __IO uint32_t *depctl_addr;
  uint32_t Status = 0;  
  
  depctl.d32 = 0;
  if (ep->is_in == 1) {
    depctl_addr = &(pdev->hw->INEP_REGS[ep->num]->DIEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);
    
    if (depctl.b.stall == 1) {
      Status = EP_TX_STALL;
    } else if (depctl.b.naksts == 1) {
      Status = EP_TX_NAK;
    } else {
      Status = EP_TX_VALID;     
    }
  } else {
    depctl_addr = &(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);
    if (depctl.b.stall == 1) {
      Status = EP_RX_STALL;
    } else if (depctl.b.naksts == 1) {
      Status = EP_RX_NAK;
    } else {
      Status = EP_RX_VALID; 
    }
  } 
  
  /* Return the current status */
  return Status;
}

/**
* @brief  Set the EP Status
* @param  pdev : Selected device
*         Status : new Status
*         ep : EP structure
* @retval : None
*/
void SetEPStatus (HANDLE *pdev , EP *ep , uint32_t Status)
{
  DEPCTL_TypeDef  depctl;
  __IO uint32_t *depctl_addr;
  
  depctl.d32 = 0;
  
  /* Process for IN endpoint */
  if (ep->is_in == 1) {
    depctl_addr = &(pdev->hw->INEP_REGS[ep->num]->DIEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);
    
    if (Status == EP_TX_STALL)  {
      EPSetStall(pdev, ep); return;
    } else if (Status == EP_TX_NAK) {
      depctl.b.snak = 1;
    } else if (Status == EP_TX_VALID) {
      if (depctl.b.stall == 1) {  
        ep->even_odd_frame = 0;
        EPClearStall(pdev, ep);
        return;
      }      
      depctl.b.cnak = 1;
      depctl.b.usbactep = 1; 
      depctl.b.epena = 1;
    } else if (Status == EP_TX_DIS) {
      depctl.b.usbactep = 0;
    }
  } else /* Process for OUT endpoint */ {
    depctl_addr = &(pdev->hw->OUTEP_REGS[ep->num]->DOEPCTL);
    depctl.d32 = READ_REG32(depctl_addr);    
    
    if (Status == EP_RX_STALL)  {
      depctl.b.stall = 1;
    } else if (Status == EP_RX_NAK) {
      depctl.b.snak = 1;
    } else if (Status == EP_RX_VALID) {
      if (depctl.b.stall == 1) {  
        ep->even_odd_frame = 0;
        EPClearStall(pdev, ep);
        return;
      }  
      depctl.b.cnak = 1;
      depctl.b.usbactep = 1;    
      depctl.b.epena = 1;
    } else if (Status == EP_RX_DIS) {
      depctl.b.usbactep = 0;    
    }
  }
  
  WRITE_REG32(depctl_addr, depctl.d32); 
}

#endif

/* THE END */
