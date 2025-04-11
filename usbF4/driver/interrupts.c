/**
  ******************************************************************************
  * @file    usb_dcd_int.c
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Peripheral Device interrupt subroutines
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */

#include "types.h"
#include "usb_conf.h"

#include "usb_core.h"
#include "protos.h"

#include "usb_regs.h"
#include "usb_defines.h"

/* From usb_dcd_int.h */
// #include "usb_dcd_int.h"
#define CLEAR_IN_EP_INTR(epnum,intr) \
  diepint.d32=0; \
  diepint.b.intr = 1; \
  WRITE_REG32(&pdev->hw->INEP_REGS[epnum]->DIEPINT,diepint.d32);

#define CLEAR_OUT_EP_INTR(epnum,intr) \
  doepint.d32=0; \
  doepint.b.intr = 1; \
  WRITE_REG32(&pdev->hw->OUTEP_REGS[(epnum)]->DOEPINT,doepint.d32);

uint32_t OTG_ISR_Handler (HANDLE *pdev);
uint32_t OTG_EP1OUT_ISR_Handler (HANDLE *pdev);
uint32_t OTG_EP1IN_ISR_Handler (HANDLE *pdev);

/* END - From usb_dcd_int.h */

typedef int IRQn_Type;

#define __NVIC_PRIO_BITS          4
#define __Vendor_SysTickConfig    1

/* static functions */
static uint32_t ReadDevInEP (HANDLE *pdev, uint8_t epnum);

/* Interrupt Handlers */
static uint32_t HandleInEP_ISR(HANDLE *pdev);
static uint32_t HandleOutEP_ISR(HANDLE *pdev);
static uint32_t HandleSof_ISR(HANDLE *pdev);

static uint32_t HandleRxStatusQueueLevel_ISR(HANDLE *pdev);
static uint32_t WriteEmptyTxFifo(HANDLE *pdev , uint32_t epnum);

static uint32_t HandleUsbReset_ISR(HANDLE *pdev);
static uint32_t HandleEnumDone_ISR(HANDLE *pdev);
static uint32_t HandleResume_ISR(HANDLE *pdev);
static uint32_t HandleUSBSuspend_ISR(HANDLE *pdev);

static uint32_t IsoINIncomplete_ISR(HANDLE *pdev);
static uint32_t IsoOUTIncomplete_ISR(HANDLE *pdev);
#ifdef VBUS_SENSING_ENABLED
static uint32_t SessionRequest_ISR(HANDLE *pdev);
static uint32_t OTG_ISR(HANDLE *pdev);
#endif


#ifdef HS_DEDICATED_EP1_ENABLED  
/**
* @brief  OTG_EP1OUT_ISR_Handler
*         handles all USB Interrupts
* @param  pdev: device instance
* @retval status
*/
uint32_t OTG_EP1OUT_ISR_Handler (HANDLE *pdev)
{
  
  DOEPINTn_TypeDef  doepint;
  DEPXFRSIZ_TypeDef  deptsiz;  
  
  doepint.d32 = READ_REG32(&pdev->hw->OUTEP_REGS[1]->DOEPINT);
  doepint.d32&= READ_REG32(&pdev->hw->DREGS->DOUTEP1MSK);

  usb_debug ( DM_ORIG, "OTG EP1 OUT ISR status: %X\n", doepint.d32 );
  
  /* Transfer complete */
  if ( doepint.b.xfercompl )
  {
    /* Clear the bit in DOEPINTn for this interrupt */
    CLEAR_OUT_EP_INTR(1, xfercompl);
    if (pdev->cfg.dma_enable == 1)
    {
      deptsiz.d32 = READ_REG32(&(pdev->hw->OUTEP_REGS[1]->DOEPTSIZ));
      pdev->dev.out_ep[1].xfer_count = pdev->dev.out_ep[1].xfer_len- \
        deptsiz.b.xfersize;
    }    
    /* Inform upper layer: data ready */
    /* RX COMPLETE */
    // INT_fops->DataOutStage(pdev , 1);
    CORE_DataOutStage(pdev , 1);
  }
  
  /* Endpoint disable  */
  if ( doepint.b.epdisabled ) {
    /* Clear the bit in DOEPINTn for this interrupt */
    CLEAR_OUT_EP_INTR(1, epdisabled);
  }

  return 1;
}

/**
* @brief  OTG_EP1IN_ISR_Handler
*         handles all USB Interrupts
* @param  pdev: device instance
* @retval status
*/
uint32_t OTG_EP1IN_ISR_Handler (HANDLE *pdev)
{
  
  DIEPINTn_TypeDef  diepint;
  uint32_t fifoemptymsk, msk, emp;
  
  msk = READ_REG32(&pdev->hw->DREGS->DINEP1MSK);
  emp = READ_REG32(&pdev->hw->DREGS->DIEPEMPMSK);
  msk |= ((emp >> 1 ) & 0x1) << 7;

  diepint.d32  = READ_REG32(&pdev->hw->INEP_REGS[1]->DIEPINT) & msk;  

  usb_debug ( DM_ORIG, "OTG EP1 IN ISR status: %X\n", diepint.d32 );
  
  if ( diepint.b.xfercompl )
  {
    fifoemptymsk = 0x1 << 1;
    MODIFY_REG32(&pdev->hw->DREGS->DIEPEMPMSK, fifoemptymsk, 0);
    CLEAR_IN_EP_INTR(1, xfercompl);
    /* TX COMPLETE */
    // INT_fops->DataInStage(pdev , 1);
    CORE_DataInStage(pdev , 1);
  }
  if ( diepint.b.epdisabled )
  {
    CLEAR_IN_EP_INTR(1, epdisabled);
  }  
  if ( diepint.b.timeout )
  {
    CLEAR_IN_EP_INTR(1, timeout);
  }
  if (diepint.b.intktxfemp)
  {
    CLEAR_IN_EP_INTR(1, intktxfemp);
  }
  if (diepint.b.inepnakeff)
  {
    CLEAR_IN_EP_INTR(1, inepnakeff);
  }
  if (diepint.b.emptyintr)
  {
    WriteEmptyTxFifo(pdev , 1);
    CLEAR_IN_EP_INTR(1, emptyintr);
  }
  return 1;
}
#endif

int tusb_int_count = 0;
int tusb_sof_count = 0;
int tusb_xof_count = 0;

/**
* @brief  STM32_USBF_OTG_ISR_Handler
*         handles all USB Interrupts
* @param  pdev: device instance
* @retval status
*/
uint32_t
OTG_ISR_Handler (HANDLE *pdev)
{
  GINTSTS_TypeDef  gintr_status;
  uint32_t retval = 0;

#ifdef notdef
  /* tjt 3-20-2025
   * The idea here is to clean up debug and eliminate messages when
   * it is only SOF interrupts, but rather than set this up so nicely
   * like this on every call, I just cheat.
   */
  GINTSTS_TypeDef  sof_status;
  sof_status.d32 = 0;
  sof_status.b.sofintr = 1;
#endif

#define	SOF_ONLY	0x8

  // Bad idea, we get non-stop SOF interrupts
  // usb_debug ( DM_ORIG, "OTG ISR called\n" );
  // printf ( "OTG ISR called\n" );

  /* Count all interrupts */
  ++tusb_int_count;
  
  if (IsDeviceMode(pdev)) /* ensure that we are in device mode */
  {
    gintr_status.d32 = ReadCoreItr(pdev);

	/* Clean up debug.
	 * This also would get swamped by endless SOF interrupts at 1000 Hz
	 * Many times we get SOF along with other bits set, and we don't
	 * want to ignore those.
	 */
	if ( gintr_status.d32 != SOF_ONLY ) {
		usb_debug ( DM_ORIG, "OTG ISR status: %X\n", gintr_status.d32 );
		//printf ( "OTG ISR status: %X\n", gintr_status.d32 );
		++tusb_xof_count;
    }

    if ( ! gintr_status.d32 ) /* avoid spurious interrupt */
      return 0;
    
    if (gintr_status.b.outepintr) {
      usb_debug ( DM_ORIG, "USBint - OUT Endpoint\n" );
      retval |= HandleOutEP_ISR(pdev);
    }    
    
    if (gintr_status.b.inepint) {
      usb_debug ( DM_ORIG, "USBint - IN Endpoint\n" );
      retval |= HandleInEP_ISR(pdev);
    }
    
    if (gintr_status.b.modemismatch) {
      GINTSTS_TypeDef  gintsts;
      
      usb_debug ( DM_ORIG, "USBint - mode MM\n" );
      /* Clear interrupt */
      gintsts.d32 = 0;
      gintsts.b.modemismatch = 1;
      WRITE_REG32(&pdev->hw->GREGS->GINTSTS, gintsts.d32);
    }
    
    if (gintr_status.b.wkupintr) {
      usb_debug ( DM_EVENT, "interrupt: resume\n" );
      retval |= HandleResume_ISR(pdev);
    }
    
    if (gintr_status.b.usbsuspend) {
      usb_debug ( DM_EVENT, "interrupt: suspend\n" );
      retval |= HandleUSBSuspend_ISR(pdev);
    }

    if (gintr_status.b.sofintr) {
	  // No!  These are continuous at 1000 Hz
      // usb_debug ( DM_EVENT, "interrupt: SOF\n" );
      retval |= HandleSof_ISR(pdev);
	  ++tusb_sof_count;
    }
    
    if (gintr_status.b.rxstsqlvl) {
      usb_debug ( DM_ORIG, "USBint - Rx level\n" );
      retval |= HandleRxStatusQueueLevel_ISR(pdev);
    }
    
    if (gintr_status.b.usbreset) {
      usb_debug ( DM_EVENT, "interrupt: reset\n" );
      retval |= HandleUsbReset_ISR(pdev);
    }

    if (gintr_status.b.enumdone) {
      usb_debug ( DM_EVENT, "interrupt - speed enumeration done\n" );
      retval |= HandleEnumDone_ISR(pdev);
	  if ( pdev->cfg.speed == SPEED_HIGH )
		  usb_debug ( DM_ENUM, "interface running at HS\n" );
	  else
		  usb_debug ( DM_ENUM, "interface running at FS\n" );
    }
    
    if (gintr_status.b.incomplisoin) {
      retval |= IsoINIncomplete_ISR(pdev);
    }

    if (gintr_status.b.incomplisoout) {
      retval |= IsoOUTIncomplete_ISR(pdev);
    }    

#ifdef VBUS_SENSING_ENABLED
    if (gintr_status.b.sessreqintr) {
      retval |= SessionRequest_ISR(pdev);
    }

    if (gintr_status.b.otgintr) {
      retval |= OTG_ISR(pdev);
    }   
#endif    
  }

  return retval;
}

#ifdef VBUS_SENSING_ENABLED
/**
* @brief  SessionRequest_ISR
*         Indicates that the USB_OTG controller has detected a connection
* @param  pdev: device instance
* @retval status
*/
static uint32_t SessionRequest_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef  gintsts;  
  // INT_fops->DevConnected (pdev);
  CORE_DevConnected (pdev);
  usb_debug ( DM_ORIG, "Event - connected\n" );

  /* Clear interrupt */
  gintsts.d32 = 0;
  gintsts.b.sessreqintr = 1;
  WRITE_REG32 (&pdev->hw->GREGS->GINTSTS, gintsts.d32);   
  return 1;
}

/**
* @brief  OTG_ISR
*         Indicates that the USB_OTG controller has detected an OTG event:
*                 used to detect the end of session i.e. disconnection
* @param  pdev: device instance
* @retval status
*/
static uint32_t OTG_ISR(HANDLE *pdev)
{

  GOTGINT_TypeDef  gotgint;

  gotgint.d32 = READ_REG32(&pdev->hw->GREGS->GOTGINT);
  
  if (gotgint.b.sesenddet) {
    // INT_fops->DevDisconnected (pdev);
    CORE_DevDisconnected (pdev);
    usb_debug ( DM_ORIG, "Event - disconnected\n" );
  }
  /* Clear OTG interrupt */
  WRITE_REG32(&pdev->hw->GREGS->GOTGINT, gotgint.d32); 
  return 1;
}
#endif
/**
* @brief  HandleResume_ISR
*         Indicates that the USB_OTG controller has detected a resume or
*                 remote Wake-up sequence
* @param  pdev: device instance
* @retval status
*/
static uint32_t HandleResume_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef  gintsts;
  DCTL_TypeDef     devctl;
  PCGCCTL_TypeDef  power;
  
  if(pdev->cfg.low_power)
  {
    /* un-gate USB Core clock */
    power.d32 = READ_REG32(pdev->hw->PCGCCTL);
    power.b.gatehclk = 0;
    power.b.stoppclk = 0;
    WRITE_REG32(pdev->hw->PCGCCTL, power.d32);
  }
  
  /* Clear the Remote Wake-up Signaling */
  devctl.d32 = 0;
  devctl.b.rmtwkupsig = 1;
  MODIFY_REG32(&pdev->hw->DREGS->DCTL, devctl.d32, 0);
  
  /* Inform upper layer by the Resume Event */
  // INT_fops->Resume (pdev);
  CORE_Resume (pdev);
  
  /* Clear interrupt */
  gintsts.d32 = 0;
  gintsts.b.wkupintr = 1;
  WRITE_REG32 (&pdev->hw->GREGS->GINTSTS, gintsts.d32);
  return 1;
}

/**
* @brief  HandleUSBSuspend_ISR
*         Indicates that SUSPEND state has been detected on the USB
* @param  pdev: device instance
* @retval status
*/
static uint32_t HandleUSBSuspend_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef  gintsts;
  PCGCCTL_TypeDef  power;
  DSTS_TypeDef     dsts;
  
  // INT_fops->Suspend (pdev);      
  CORE_Suspend (pdev);      
  
  dsts.d32 = READ_REG32(&pdev->hw->DREGS->DSTS);
    
  /* Clear interrupt */
  gintsts.d32 = 0;
  gintsts.b.usbsuspend = 1;
  WRITE_REG32(&pdev->hw->GREGS->GINTSTS, gintsts.d32);
  
#ifndef HYDRA
  if((pdev->cfg.low_power) && (dsts.b.suspsts == 1))
  {
	/*  switch-off the clocks */
    power.d32 = 0;
    power.b.stoppclk = 1;
    MODIFY_REG32(pdev->hw->PCGCCTL, 0, power.d32);  
    
    power.b.gatehclk = 1;
    MODIFY_REG32(pdev->hw->PCGCCTL, 0, power.d32);
    
    /* Request to enter Sleep mode after exit from current ISR */
    SCB->SCR |= (SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
  }
#endif

  return 1;
}

/**
* @brief  HandleInEP_ISR
*         Indicates that an IN EP has a pending Interrupt
* @param  pdev: device instance
* @retval status
*/
static uint32_t HandleInEP_ISR(HANDLE *pdev)
{
  DIEPINTn_TypeDef  diepint;
  
  uint32_t ep_intr;
  uint32_t epnum = 0;
  uint32_t fifoemptymsk;
  diepint.d32 = 0;
  ep_intr = ReadDevAllInEPItr(pdev);
  
  while ( ep_intr )
  {
    if (ep_intr&0x1) {
      // usb_debug ( DM_ORIG, "USBint = IN Endpoint %d\n", epnum );

      diepint.d32 = ReadDevInEP(pdev , epnum); /* Get In ITR status */

      if ( diepint.b.xfercompl ) {
	usb_debug ( DM_ORIG, "USBint = IN Endpoint %d Xfer complete\n", epnum );
        fifoemptymsk = 0x1 << epnum;
        MODIFY_REG32(&pdev->hw->DREGS->DIEPEMPMSK, fifoemptymsk, 0);
        CLEAR_IN_EP_INTR(epnum, xfercompl);
        /* TX COMPLETE */
        // INT_fops->DataInStage(pdev , epnum);
        CORE_DataInStage(pdev , epnum);
        
        if (pdev->cfg.dma_enable == 1) {
          if((epnum == 0) && (pdev->dev.device_state == EP0_STATUS_IN))
          {
            /* prepare to rx more setup packets */
            EP0_OutStart(pdev);
          }
        }           
      }

      if ( diepint.b.timeout ) {
        CLEAR_IN_EP_INTR(epnum, timeout);
      }
      if (diepint.b.intktxfemp) {
        CLEAR_IN_EP_INTR(epnum, intktxfemp);
      }
      if (diepint.b.inepnakeff) {
        CLEAR_IN_EP_INTR(epnum, inepnakeff);
      }
      if ( diepint.b.epdisabled ) {
        CLEAR_IN_EP_INTR(epnum, epdisabled);
      }       

      if (diepint.b.emptyintr) {
	usb_debug ( DM_ORIG, "USBint = IN Endpoint %d empty Tx\n", epnum );
        WriteEmptyTxFifo(pdev , epnum);
        CLEAR_IN_EP_INTR(epnum, emptyintr);
      }
    }
    epnum++;
    ep_intr >>= 1;
  }
  
  return 1;
}

/**
* @brief  HandleOutEP_ISR
*         Indicates that an OUT EP has a pending Interrupt
* @param  pdev: device instance
* @retval status
*/
static uint32_t
HandleOutEP_ISR(HANDLE *pdev)
{
  uint32_t ep_intr;
  DOEPINTn_TypeDef  doepint;
  DEPXFRSIZ_TypeDef  deptsiz;
  uint32_t epnum = 0;
  
  doepint.d32 = 0;
  
  /* Read in the device interrupt bits */
  ep_intr = ReadDevAllOutEp_itr(pdev);
  
  while ( ep_intr ) {
    if (ep_intr&0x1) {

      // usb_debug ( DM_ORIG, "USBint = OUT Endpoint %d\n", epnum );
      
      doepint.d32 = ReadDevOutEP_itr(pdev, epnum);
      
      /* Transfer complete */
      if ( doepint.b.xfercompl ) {
	  usb_debug ( DM_ORIG, "USBint = OUT Endpoint %d xfer complete\n", epnum );

        /* Clear the bit in DOEPINTn for this interrupt */
        CLEAR_OUT_EP_INTR(epnum, xfercompl);
        if (pdev->cfg.dma_enable == 1) {
          deptsiz.d32 = READ_REG32(&(pdev->hw->OUTEP_REGS[epnum]->DOEPTSIZ));
          /*ToDo : handle more than one single MPS size packet */
          pdev->dev.out_ep[epnum].xfer_count = pdev->dev.out_ep[epnum].maxpacket - \
            deptsiz.b.xfersize;
        }

        /* Inform upper layer: data ready */
        /* RX COMPLETE */
        // INT_fops->DataOutStage(pdev , epnum);
        CORE_DataOutStage(pdev , epnum);
        
        if (pdev->cfg.dma_enable == 1) {
          if((epnum == 0) && (pdev->dev.device_state == EP0_STATUS_OUT)) {
            /* prepare to rx more setup packets */
            EP0_OutStart(pdev);
          }
        }        
      }

      /* Endpoint disable  */
      if ( doepint.b.epdisabled ) {
	usb_debug ( DM_ORIG, "USBint = OUT Endpoint %d disable\n", epnum );
        /* Clear the bit in DOEPINTn for this interrupt */
        CLEAR_OUT_EP_INTR(epnum, epdisabled);
      }

      /* Setup Phase Done (control EPs) */
      if ( doepint.b.setup ) {
		usb_debug ( DM_ORIG, "USBint = OUT Endpoint %d setup done\n", epnum );
        
        /* inform the upper layer that a setup packet is available */
        /* SETUP COMPLETE */
        // INT_fops->SetupStage(pdev);
        CORE_SetupStage(pdev);
        CLEAR_OUT_EP_INTR(epnum, setup);
      }

    }
    epnum++;
    ep_intr >>= 1;
  }
  return 1;
}

/**
* @brief  HandleSof_ISR
*         Handles the SOF Interrupts
* @param  pdev: device instance
* @retval status
*/
static uint32_t HandleSof_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef  GINTSTS;
  
  // INT_fops->SOF(pdev);
  CORE_SOF(pdev);
  
  /* Clear interrupt */
  GINTSTS.d32 = 0;
  GINTSTS.b.sofintr = 1;
  WRITE_REG32 (&pdev->hw->GREGS->GINTSTS, GINTSTS.d32);
  
  return 1;
}

/**
* @brief  HandleRxStatusQueueLevel_ISR
*         Handles the Rx Status Queue Level Interrupt
* @param  pdev: device instance
* @retval status
*/
static uint32_t
HandleRxStatusQueueLevel_ISR(HANDLE *pdev)
{
  GINTMSK_TypeDef  int_mask;
  DRXSTS_TypeDef   status;
  EP *ep;
  
  /* Disable the Rx Status Queue Level interrupt */
  int_mask.d32 = 0;
  int_mask.b.rxstsqlvl = 1;
  MODIFY_REG32( &pdev->hw->GREGS->GINTMSK, int_mask.d32, 0);
  
  /* Get the Status from the top of the FIFO */
  status.d32 = READ_REG32( &pdev->hw->GREGS->GRXSTSP );
  
  ep = &pdev->dev.out_ep[status.b.epnum];
  
  switch (status.b.pktsts)
  {
  case STS_GOUT_NAK:
    break;
  case STS_DATA_UPDT:
    if (status.b.bcnt)
    {
	  usb_debug ( DM_ORIG, "Endpoint %d read DATA packet %d bytes from FIFO\n", status.b.epnum, status.b.bcnt );
      ReadPacket(pdev,ep->xfer_buff, status.b.bcnt);
      ep->xfer_buff += status.b.bcnt;
      ep->xfer_count += status.b.bcnt;
    }
    break;
  case STS_XFER_COMP:
    break;
  case STS_SETUP_COMP:
    break;
  case STS_SETUP_UPDT:
    /* Copy the setup packet received in FIFO into the setup buffer in RAM */
	usb_debug ( DM_ORIG, "Endpoint %d read SETUP packet %d bytes from FIFO\n", status.b.epnum, status.b.bcnt );
    ReadPacket(pdev , pdev->dev.setup_packet, 8);
	usb_dump ( DM_ENUM, "Rx setup", pdev->dev.setup_packet, 8 );
    ep->xfer_count += status.b.bcnt;
    break;
  default:
    break;
  }
  
  /* Enable the Rx Status Queue Level interrupt */
  MODIFY_REG32( &pdev->hw->GREGS->GINTMSK, 0, int_mask.d32);
  
  return 1;
}

/**
* @brief  WriteEmptyTxFifo
*         check FIFO for the next packet to be loaded
* @param  pdev: device instance
* @retval status
*/
static uint32_t 
WriteEmptyTxFifo(HANDLE *pdev, uint32_t epnum)
{
  DTXFSTSn_TypeDef  txstatus;
  EP *ep;
  uint32_t len = 0;
  uint32_t len32b;
  txstatus.d32 = 0;
  
  ep = &pdev->dev.in_ep[epnum];
  
  len = ep->xfer_len - ep->xfer_count;
  
  if (len > ep->maxpacket)
    len = ep->maxpacket;
  
  len32b = (len + 3) / 4;
  txstatus.d32 = READ_REG32( &pdev->hw->INEP_REGS[epnum]->DTXFSTS);
  
  while  (txstatus.b.txfspcavail > len32b &&
          ep->xfer_count < ep->xfer_len &&
            ep->xfer_len != 0) {
    /* Write the FIFO */
    len = ep->xfer_len - ep->xfer_count;
    
    if (len > ep->maxpacket)
      len = ep->maxpacket;

    len32b = (len + 3) / 4;
    
    WritePacket (pdev , ep->xfer_buff, epnum, len);
    
    ep->xfer_buff  += len;
    ep->xfer_count += len;
    
    if( ep->xfer_count >= ep->xfer_len) {
      uint32_t fifoemptymsk = 1 << ep->num;
      MODIFY_REG32(&pdev->hw->DREGS->DIEPEMPMSK, fifoemptymsk, 0);
      break;
    }

    txstatus.d32 = READ_REG32(&pdev->hw->INEP_REGS[epnum]->DTXFSTS);
  }
  
  return 1;
}

/**
* @brief  HandleUsbReset_ISR
*         This interrupt occurs when a USB Reset is detected
* @param  pdev: device instance
* @retval status
*/
static uint32_t HandleUsbReset_ISR(HANDLE *pdev)
{
  DAINT_TypeDef    daintmsk;
  DOEPMSK_TypeDef  doepmsk;
  DIEPMSK_TypeDef  diepmsk;
  DCFG_TypeDef     dcfg;
  DCTL_TypeDef     dctl;
  GINTSTS_TypeDef  gintsts;
  uint32_t i;
  
  dctl.d32 = 0;
  daintmsk.d32 = 0;
  doepmsk.d32 = 0;
  diepmsk.d32 = 0;
  dcfg.d32 = 0;
  gintsts.d32 = 0;
  
  /* Clear the Remote Wake-up Signaling */
  dctl.b.rmtwkupsig = 1;
  MODIFY_REG32(&pdev->hw->DREGS->DCTL, dctl.d32, 0 );
  
  /* Flush the Tx FIFO */
  FlushTxFifo(pdev ,  0 );
  
  for (i = 0; i < pdev->cfg.dev_endpoints ; i++)
  {
    WRITE_REG32( &pdev->hw->INEP_REGS[i]->DIEPINT, 0xFF);
    WRITE_REG32( &pdev->hw->OUTEP_REGS[i]->DOEPINT, 0xFF);
  }
  WRITE_REG32( &pdev->hw->DREGS->DAINT, 0xFFFFFFFF );
  
  daintmsk.ep.in = 1;
  daintmsk.ep.out = 1;
  WRITE_REG32( &pdev->hw->DREGS->DAINTMSK, daintmsk.d32 );
  
  doepmsk.b.setup = 1;
  doepmsk.b.xfercompl = 1;
  doepmsk.b.epdisabled = 1;
  WRITE_REG32( &pdev->hw->DREGS->DOEPMSK, doepmsk.d32 );
#ifdef HS_DEDICATED_EP1_ENABLED   
  WRITE_REG32( &pdev->hw->DREGS->DOUTEP1MSK, doepmsk.d32 );
#endif
  diepmsk.b.xfercompl = 1;
  diepmsk.b.timeout = 1;
  diepmsk.b.epdisabled = 1;

  WRITE_REG32( &pdev->hw->DREGS->DIEPMSK, diepmsk.d32 );
#ifdef HS_DEDICATED_EP1_ENABLED  
  WRITE_REG32( &pdev->hw->DREGS->DINEP1MSK, diepmsk.d32 );
#endif
  /* Reset Device Address */
  dcfg.d32 = READ_REG32( &pdev->hw->DREGS->DCFG);
  dcfg.b.devaddr = 0;
  WRITE_REG32( &pdev->hw->DREGS->DCFG, dcfg.d32);
  
  
  /* setup EP0 to receive SETUP packets */
  EP0_OutStart(pdev);
  
  /* Clear interrupt */
  gintsts.d32 = 0;
  gintsts.b.usbreset = 1;
  WRITE_REG32 (&pdev->hw->GREGS->GINTSTS, gintsts.d32);
  
  /*Reset internal state machine */
  // INT_fops->Reset(pdev);
  CORE_Reset(pdev);
  return 1;
}

/**
* This is speed enumeration
* @brief  HandleEnumDone_ISR
*         Read the device status register and set the device speed
* @param  pdev: device instance
* @retval status
*/
static uint32_t
HandleEnumDone_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef  gintsts;
  GUSBCFG_TypeDef  gusbcfg;
  
  EP0Activate(pdev);
  
  /* Set USB turn-around time based on device speed and PHY interface. */
  gusbcfg.d32 = READ_REG32(&pdev->hw->GREGS->GUSBCFG);
  
  /* Full or High speed */
  if ( GetDeviceSpeed(pdev) == USB_SPEED_HIGH) {
    pdev->cfg.speed            = SPEED_HIGH;
    pdev->cfg.mps              = HS_MAX_PACKET_SIZE ;
    gusbcfg.b.usbtrdtim = 9;
  } else {
    pdev->cfg.speed            = SPEED_FULL;
    pdev->cfg.mps              = FS_MAX_PACKET_SIZE ;
    gusbcfg.b.usbtrdtim = 5;
  }
  
  WRITE_REG32(&pdev->hw->GREGS->GUSBCFG, gusbcfg.d32);
  
  /* Clear interrupt */
  gintsts.d32 = 0;
  gintsts.b.enumdone = 1;
  WRITE_REG32( &pdev->hw->GREGS->GINTSTS, gintsts.d32 );

  return 1;
}


/**
* @brief  IsoINIncomplete_ISR
*         handle the ISO IN incomplete interrupt
* @param  pdev: device instance
* @retval status
*/
static uint32_t IsoINIncomplete_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef gintsts;  
  
  gintsts.d32 = 0;

  // INT_fops->IsoINIncomplete (pdev); 
  CORE_IsoINIncomplete (pdev); 
  
  /* Clear interrupt */
  gintsts.b.incomplisoin = 1;
  WRITE_REG32(&pdev->hw->GREGS->GINTSTS, gintsts.d32);
  
  return 1;
}

/**
* @brief  IsoOUTIncomplete_ISR
*         handle the ISO OUT incomplete interrupt
* @param  pdev: device instance
* @retval status
*/
static uint32_t IsoOUTIncomplete_ISR(HANDLE *pdev)
{
  GINTSTS_TypeDef gintsts;  
  
  gintsts.d32 = 0;

  // INT_fops->IsoOUTIncomplete (pdev); 
  CORE_IsoOUTIncomplete (pdev); 
  
  /* Clear interrupt */
  gintsts.b.incomplisoout = 1;
  WRITE_REG32(&pdev->hw->GREGS->GINTSTS, gintsts.d32);
  return 1;
}
/**
* @brief  ReadDevInEP
*         Reads ep flags
* @param  pdev: device instance
* @retval status
*/
static uint32_t ReadDevInEP (HANDLE *pdev, uint8_t epnum)
{
  uint32_t v, msk, emp;
  msk = READ_REG32(&pdev->hw->DREGS->DIEPMSK);
  emp = READ_REG32(&pdev->hw->DREGS->DIEPEMPMSK);
  msk |= ((emp >> epnum) & 0x1) << 7;
  v = READ_REG32(&pdev->hw->INEP_REGS[epnum]->DIEPINT) & msk;
  return v;
}

/* THE END */
