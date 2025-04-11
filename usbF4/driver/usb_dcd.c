/**
  ******************************************************************************
  * @file    usb_dcd.c
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Peripheral Device Interface Layer
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */

#include "types.h"
#include "usb_conf.h"

#include "usb_core.h"

#include "usb_regs.h"
#include "usb_defines.h"

#include "usb_dcd.h"

void
Init(HANDLE *pdev , CORE_ID_TypeDef coreID)
{
  uint32_t i;
  EP *ep;
  
  SelectCore (pdev , coreID);

  usb_debug ( DM_EVENT, "Event - init\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
  
  pdev->dev.device_status = DEFAULT;
  pdev->dev.device_address = 0;

  /* Init ep structure for IN endpoints*/
  for (i = 0; i < pdev->cfg.dev_endpoints ; i++) {
    ep = &pdev->dev.in_ep[i];
    /* Init ep structure */
    ep->is_in = 1;
    ep->num = i;
    ep->tx_fifo_num = i;
    /* Control until ep is activated */
    ep->type = EP_TYPE_CTRL;
    ep->maxpacket =  MAX_EP0_SIZE;
    ep->xfer_buff = 0;
    ep->xfer_len = 0;
  }
  usb_debug ( DM_EVENT, "Event - init A\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
  
  /* Init ep structure for OUT endpoints*/
  for (i = 0; i < pdev->cfg.dev_endpoints; i++) {
    ep = &pdev->dev.out_ep[i];
    /* Init ep structure */
    ep->is_in = 0;	/* <<< XXX */
    ep->num = i;
    ep->tx_fifo_num = i;
    /* Control until ep is activated */
    ep->type = EP_TYPE_CTRL;
    ep->maxpacket = MAX_EP0_SIZE;
    ep->xfer_buff = 0;
    ep->xfer_len = 0;
  }
  usb_debug ( DM_EVENT, "Event - init B\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
  
  DisableGlobalInt(pdev);

  /* Force Device Mode*/
  SetCurrentMode(pdev, DEVICE_MODE);
  
  /*Init the Core (common init.) */
  CoreInit(pdev);
  usb_debug ( DM_EVENT, "Event - init C\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );

/* XXX - why is this order so special ?? */
#ifndef HYDRA
#if defined (STM32F446xx) || defined (STM32F469_479xx)
  /* Force Device Mode*/
  SetCurrentMode(pdev, DEVICE_MODE);
  
  /*Init the Core (common init.) */
  CoreInit(pdev);
#else
    /*Init the Core (common init.) */
  CoreInit(pdev);

  /* Force Device Mode*/
  SetCurrentMode(pdev, DEVICE_MODE);
#endif
#endif /* HYDRA */
  usb_debug ( DM_EVENT, "Event - init D\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
  
  /* Init Device */
  CoreInitDev(pdev);
  usb_debug ( DM_EVENT, "Event - init E\n" );
	printf ( "dev endpoints: %d\n", pdev->cfg.dev_endpoints );
  
  /* Enable USB Global interrupt */
  EnableGlobalInt(pdev);
  usb_debug ( DM_EVENT, "Event - init F\n" );
}

/**
* @brief  Configure an EP
* @param pdev : Device instance
* @param epdesc : Endpoint Descriptor
* @retval : status
*/
uint32_t
EP_Open(HANDLE *pdev , 
                     uint8_t ep_addr,
                     uint16_t ep_mps,
                     uint8_t ep_type)
{
  EP *ep;
  
  if ((ep_addr & 0x80) == 0x80)
  {
    ep = &pdev->dev.in_ep[ep_addr & 0x7F];
  }
  else
  {
    ep = &pdev->dev.out_ep[ep_addr & 0x7F];
  }
  ep->num   = ep_addr & 0x7F;
  
  ep->is_in = (0x80 & ep_addr) != 0;
  ep->maxpacket = ep_mps;
  ep->type = ep_type;
  if (ep->is_in)
  {
    /* Assign a Tx FIFO */
    ep->tx_fifo_num = ep->num;
  }
  /* Set initial data PID. */
  if (ep_type == EP_BULK )
  {
    ep->data_pid_start = 0;
  }
  EPActivate(pdev , ep );
  return 0;
}
/**
* @brief  called when an EP is disabled
* @param pdev: device instance
* @param ep_addr: endpoint address
* @retval : status
*/
uint32_t
EP_Close(HANDLE *pdev , uint8_t  ep_addr)
{
  EP *ep;
  
  if ((ep_addr&0x80) == 0x80) {
    ep = &pdev->dev.in_ep[ep_addr & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[ep_addr & 0x7F];
  }
  ep->num   = ep_addr & 0x7F;
  ep->is_in = (0x80 & ep_addr) != 0;
  EPDeactivate(pdev , ep );
  return 0;
}


/**
* @brief  EP_PrepareRx
* @param pdev: device instance
* @param ep_addr: endpoint address
* @param pbuf: pointer to Rx buffer
* @param buf_len: data length
* @retval : status
*/
uint32_t
EP_PrepareRx( HANDLE *pdev,
                            uint8_t   ep_addr,
                            uint8_t *pbuf,                        
                            uint16_t  buf_len)
{
  EP *ep;
  
  ep = &pdev->dev.out_ep[ep_addr & 0x7F];
  
  /*setup and start the Xfer */
  ep->xfer_buff = pbuf;  
  ep->xfer_len = buf_len;
  ep->xfer_count = 0;
  ep->is_in = 0;
  ep->num = ep_addr & 0x7F;
  
  if (pdev->cfg.dma_enable == 1) {
    ep->dma_addr = (uint32_t)pbuf;  
  }
  
  if ( ep->num == 0 ) {
    EP0StartXfer(pdev , ep);
  } else {
    EPStartXfer(pdev, ep );
  }

  return 0;
}

/**
* @brief  Transmit data over USB
* @param pdev: device instance
* @param ep_addr: endpoint address
* @param pbuf: pointer to Tx buffer
* @param buf_len: data length
* @retval : status
*/
uint32_t
EP_Tx ( HANDLE *pdev,
                     uint8_t   ep_addr,
                     uint8_t   *pbuf,
                     uint32_t   buf_len)
{
  EP *ep;
  
  ep = &pdev->dev.in_ep[ep_addr & 0x7F];
  
  /* Setup and start the Transfer */
  ep->is_in = 1;
  ep->num = ep_addr & 0x7F;  
  ep->xfer_buff = pbuf;
  ep->dma_addr = (uint32_t)pbuf;  
  ep->xfer_count = 0;
  ep->xfer_len  = buf_len;
  
  if ( ep->num == 0 ) {
    EP0StartXfer(pdev , ep);
  } else {
    EPStartXfer(pdev, ep );
  }
  return 0;
}


/**
* @brief  Stall an endpoint.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
uint32_t
EP_Stall (HANDLE *pdev, uint8_t   epnum)
{
  EP *ep;

  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }

  ep->is_stall = 1;
  ep->num   = epnum & 0x7F;
  ep->is_in = ((epnum & 0x80) == 0x80);
  
  EPSetStall(pdev , ep);
  return (0);
}


/**
* @brief  Clear stall condition on endpoints.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
uint32_t
EP_ClrStall (HANDLE *pdev, uint8_t epnum)
{
  EP *ep;
  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];    
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }
  
  ep->is_stall = 0;  
  ep->num   = epnum & 0x7F;
  ep->is_in = ((epnum & 0x80) == 0x80);
  
  EPClearStall(pdev , ep);
  return (0);
}


/**
* @brief  This Function flushes the FIFOs.
* @param pdev: device instance
* @param epnum: endpoint address
* @retval : status
*/
uint32_t
EP_Flush (HANDLE *pdev , uint8_t epnum)
{

  if ((epnum & 0x80) == 0x80) {
    FlushTxFifo(pdev, epnum & 0x7F);
  } else {
    FlushRxFifo(pdev);
  }

  return (0);
}


/**
* @brief  This Function set USB device address
* @param pdev: device instance
* @param address: new device address
* @retval : status
*/
void
EP_SetAddress (HANDLE *pdev, uint8_t address)
{
  DCFG_TypeDef  dcfg;
  dcfg.d32 = 0;
  dcfg.b.devaddr = address;
  // MODIFY_REG32( &pdev->regs.DREGS->DCFG, 0, dcfg.d32);
  MODIFY_REG32( &pdev->hw->DREGS->DCFG, 0, dcfg.d32);
}

/**
* @brief  Connect device (enable internal pull-up)
* @param pdev: device instance
* @retval : None
*/
void
DevConnect (HANDLE *pdev)
{
#ifndef USE_OTG_MODE
  DCTL_TypeDef  dctl;
  // dctl.d32 = READ_REG32(&pdev->regs.DREGS->DCTL);
  dctl.d32 = READ_REG32(&pdev->hw->DREGS->DCTL);
  /* Connect device */
  dctl.b.sftdiscon  = 0;
  WRITE_REG32(&pdev->hw->DREGS->DCTL, dctl.d32);
  // board_mDelay(3);
  delay_ms ( 3 );
#endif
}


/**
* @brief  Disconnect device (disable internal pull-up)
* @param pdev: device instance
* @retval : None
*/
void
DevDisconnect (HANDLE *pdev)
{
#ifndef USE_OTG_MODE
  DCTL_TypeDef  dctl;
  dctl.d32 = READ_REG32(&pdev->hw->DREGS->DCTL);
  /* Disconnect device for 3ms */
  dctl.b.sftdiscon  = 1;
  WRITE_REG32(&pdev->hw->DREGS->DCTL, dctl.d32);
  // board_mDelay(3);
  delay_ms ( 3 );
#endif
}


/**
* @brief  returns the EP Status
* @param  pdev : Selected device
*         epnum : endpoint address
* @retval : EP status
*/

uint32_t
GetEPStatus(HANDLE *pdev ,uint8_t epnum)
{
  EP *ep;
  uint32_t Status = 0;  
  
  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];    
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }
  
  Status = DRV_GetEPStatus(pdev ,ep);

  /* Return the current status */
  return Status;
}

/**
* @brief  Set the EP Status
* @param  pdev : Selected device
*         Status : new Status
*         epnum : EP address
* @retval : None
*/
void
SetEPStatus (HANDLE *pdev , uint8_t epnum , uint32_t Status)
{
  EP *ep;
  
  if ((0x80 & epnum) == 0x80) {
    ep = &pdev->dev.in_ep[epnum & 0x7F];    
  } else {
    ep = &pdev->dev.out_ep[epnum];
  }
  
   DRV_SetEPStatus(pdev ,ep , Status);
}

/* THE END */
