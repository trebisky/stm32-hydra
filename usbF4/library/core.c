/**
  ******************************************************************************
  * @file    usbd_core.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011 
  * @brief   This file provides all the USBD core functions.
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */ 

#include "types.h"
#include "usb_conf.h"

#include "usb_std.h"

#include "usb_core.h"
#include "protos.h"

/* These macros and functions replace USR_cb and all the stuff
 * in the file usbd_usr.c -- all to set up this redundant status
 * variable.
 */
static volatile uint8_t usb_status;

/* XXX - note that there is also this, which seems to sort
 * of redundantly duplicate our little status variable.
 *   pdev->dev.device_status = USB_OTG_DEFAULT;
 */

#define ST_CONFIGURED 	0x01
#define ST_CONNECTED 	0x02
#define ST_RESUMED 		0x04

#define STATUS_Set(bit)		(usb_status |= bit)
#define STATUS_Clear(bit)	(usb_status &= ~bit)
#define STATUS_Reset()		(usb_status =0)

// inline void  STATUS_Reset() { (usb_status = 0); }
// void  STATUS_Reset() { (usb_status = 0); }

/* Accessor functions for the status */

uint8_t usb_isConfigured(void) { return ( usb_status & ST_CONFIGURED ); }

#ifdef VBUS_SENSING_ENABLED
uint8_t usb_isConnected(void) { return ( usb_status & ST_CONNECTED ); }
#else
uint8_t usb_isConnected(void) { return ST_CONNECTED; }
#endif

/* ################################################################## */
/* ################################################################## */
/* ################################################################## */

void
USBD_Init ( USB_OTG_CORE_HANDLE *pdev, USB_OTG_CORE_ID_TypeDef coreID )
{
  /* No harm to call this, but it does nothing */
  // USBD_DeInit(pdev);
  
  /* set USB OTG core params */
  DCD_Init(pdev , coreID);
  
  /* Upon Init call usr callback */
  /* XXX should clean this up */
  STATUS_Reset();
}

USBD_Status
USBD_DeInit ( USB_OTG_CORE_HANDLE *pdev )
{
  return USBD_OK;
}

USBD_Status
USBD_DeInitFull ( USB_OTG_CORE_HANDLE *pdev )
{
  /* Software Init */
  return USBD_OK;
}

/* ################################################################## */
/* ################################################################## */
/* ################################################################## */

/**
* @brief  USBD_SetupStage 
*         Handle the setup stage
* @param  pdev: device instance
* @retval status
*/
uint8_t
CORE_SetupStage(USB_OTG_CORE_HANDLE *pdev)
{
  USB_SETUP_REQ req;
  
  USBD_ParseSetupRequest(pdev , &req);
  
  switch (req.bmRequest & 0x1F) 
  {
  case USB_REQ_RECIPIENT_DEVICE:   
	usb_debug ( DM_ENUM, "Setup device request\n" );
    USBD_StdDevReq (pdev, &req);
    break;
    
  case USB_REQ_RECIPIENT_INTERFACE:     
	usb_debug ( DM_ENUM, "Setup interface request\n" );
    USBD_StdItfReq(pdev, &req);
    break;
    
  case USB_REQ_RECIPIENT_ENDPOINT:        
	usb_debug ( DM_ENUM, "Setup Endpoint request\n" );
    USBD_StdEPReq(pdev, &req);   
    break;
    
  default:           
	usb_debug ( DM_ENUM, "Setup stall\n" );
    DCD_EP_Stall(pdev , req.bmRequest & 0x80);
    break;
  }  
  return USBD_OK;
}

/**
* @brief  USBD_DataOutStage 
*         Handle data out stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
// static uint8_t
// USBD_DataOutStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum)
uint8_t
CORE_DataOutStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum)
{
  USB_OTG_EP *ep;
  
  // printf ( "DataOutStage, epnum = %d\n", epnum );

  if(epnum == 0) 
  {
    ep = &pdev->dev.out_ep[0];
    if ( pdev->dev.device_state == USB_OTG_EP0_DATA_OUT)
    {
      if(ep->rem_data_len > ep->maxpacket)
      {
        ep->rem_data_len -=  ep->maxpacket;
        
        if(pdev->cfg.dma_enable == 1)
        {
          /* in slave mode this, is handled by the RxSTSQLvl ISR */
          ep->xfer_buff += ep->maxpacket; 
        }        
        USBD_CtlContinueRx (pdev, 
                            ep->xfer_buff,
                            MIN(ep->rem_data_len ,ep->maxpacket));
      } else {
        // if((pdev->dev.class_cb->EP0_RxReady != NULL)&&
        //    (pdev->dev.device_status == USB_OTG_CONFIGURED)) {
        if ( pdev->dev.device_status == USB_OTG_CONFIGURED ) {
          // pdev->dev.class_cb->EP0_RxReady(pdev); 
          CLASS_EP0_RxReady(pdev); 
        }
        USBD_CtlSendStatus(pdev);
      }
    }
  }
  // else if((pdev->dev.class_cb->DataOut != NULL)&&
  //         (pdev->dev.device_status == USB_OTG_CONFIGURED)) {
  else if (pdev->dev.device_status == USB_OTG_CONFIGURED) {
    // pdev->dev.class_cb->DataOut(pdev, epnum); 
    CLASS_DataOut(pdev, epnum); 
  }
  return USBD_OK;
}

/**
* @brief  USBD_DataInStage 
*         Handle data in stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
uint8_t
CORE_DataInStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum)
{
  USB_OTG_EP *ep;
  
  if(epnum == 0) 
  {
    ep = &pdev->dev.in_ep[0];
    if ( pdev->dev.device_state == USB_OTG_EP0_DATA_IN)
    {
      if(ep->rem_data_len > ep->maxpacket)
      {
        ep->rem_data_len -=  ep->maxpacket;
        if(pdev->cfg.dma_enable == 1)
        {
          /* in slave mode this, is handled by the TxFifoEmpty ISR */
          ep->xfer_buff += ep->maxpacket;
        }
        USBD_CtlContinueSendData (pdev, 
                                  ep->xfer_buff, 
                                  ep->rem_data_len);
      }
      else
      { /* last packet is MPS multiple, so send ZLP packet */
        if((ep->total_data_len % ep->maxpacket == 0) &&
           (ep->total_data_len >= ep->maxpacket) &&
             (ep->total_data_len < ep->ctl_data_len )) {
          
          USBD_CtlContinueSendData(pdev , NULL, 0);
          ep->ctl_data_len = 0;
        } else {
          // if((pdev->dev.class_cb->EP0_TxSent != NULL)&&
          //    (pdev->dev.device_status == USB_OTG_CONFIGURED)) {
          if ( pdev->dev.device_status == USB_OTG_CONFIGURED ) {
            // pdev->dev.class_cb->EP0_TxSent(pdev); 
            CLASS_EP0_TxSent ( pdev ); 
          }          
          USBD_CtlReceiveStatus(pdev);
        }
      }
    }
  }
  // else if((pdev->dev.class_cb->DataIn != NULL)&& 
  //         (pdev->dev.device_status == USB_OTG_CONFIGURED)) {
  else if (pdev->dev.device_status == USB_OTG_CONFIGURED) {
    // pdev->dev.class_cb->DataIn(pdev, epnum); 
    CLASS_DataIn(pdev, epnum); 
  }
  return USBD_OK;
}

uint8_t
CORE_Reset(USB_OTG_CORE_HANDLE  *pdev)
{
  /* Open EP0 OUT */
  DCD_EP_Open(pdev,
              0x00,
              USB_OTG_MAX_EP0_SIZE,
              EP_TYPE_CTRL);
  
  /* Open EP0 IN */
  DCD_EP_Open(pdev,
              0x80,
              USB_OTG_MAX_EP0_SIZE,
              EP_TYPE_CTRL);
  
  /* Upon Reset call usr call back */
  pdev->dev.device_status = USB_OTG_DEFAULT;
  // pdev->dev.usr_cb->DeviceReset(pdev->cfg.speed);
  STATUS_Reset();
  usb_debug ( DM_EVENT, "Reset\n" );
  
  return USBD_OK;
}

uint8_t
CORE_Resume(USB_OTG_CORE_HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceResumed(); 
  STATUS_Set ( ST_RESUMED );
  usb_debug ( DM_EVENT, "Resumed\n" );
  pdev->dev.device_status = USB_OTG_CONFIGURED;  
  return USBD_OK;
}

uint8_t
CORE_Suspend(USB_OTG_CORE_HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceSuspended(); 
  STATUS_Clear ( ST_RESUMED );
  usb_debug ( DM_EVENT, "Suspended\n" );
  pdev->dev.device_status  = USB_OTG_SUSPENDED;
  return USBD_OK;
}

uint8_t
CORE_SOF(USB_OTG_CORE_HANDLE  *pdev)
{
  // if(pdev->dev.class_cb->SOF) {
  //   pdev->dev.class_cb->SOF(pdev); 
  // }
  CLASS_SOF(pdev); 
  return USBD_OK;
}

USBD_Status
USBD_SetCfg(USB_OTG_CORE_HANDLE  *pdev, uint8_t cfgidx)
{
  // pdev->dev.class_cb->Init(pdev, cfgidx); 
  CLASS_Init(pdev, cfgidx); 
  
  /* Upon set config call usr call back */
  // pdev->dev.usr_cb->DeviceConfigured();
  STATUS_Set ( ST_CONFIGURED );
  usb_debug ( DM_EVENT, "Configured\n" );
  return USBD_OK; 
}

USBD_Status
USBD_ClrCfg(USB_OTG_CORE_HANDLE  *pdev, uint8_t cfgidx)
{
  // pdev->dev.class_cb->DeInit(pdev, cfgidx);   
  CLASS_DeInit(pdev, cfgidx);   
  return USBD_OK;
}

 uint8_t
CORE_IsoINIncomplete(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoINIncomplete(pdev);   
  CLASS_IsoINIncomplete ( pdev );   
  return USBD_OK;
}

uint8_t
CORE_IsoOUTIncomplete(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoOUTIncomplete(pdev);   
  CLASS_IsoOUTIncomplete ( pdev );   
  return USBD_OK;
}

#ifdef VBUS_SENSING_ENABLED
uint8_t
CORE_DevConnected(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceConnected();
  STATUS_Set ( ST_CONNECTED );
  usb_debug ( DM_EVENT, "Connected\n" );
  return USBD_OK;
}

uint8_t
CORE_DevDisconnected(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceDisconnected();
  STATUS_Clear ( ST_CONNECTED );
  usb_debug ( DM_EVENT, "Disconnected\n" );
  // pdev->dev.class_cb->DeInit(pdev, 0);
  CLASS_DeInit(pdev, 0);
  return USBD_OK;
}
#endif

/* ################################################################## */
/* ################################################################## */
/* ################################################################## */

/* Was usbd_ioreq.c
 * -- a bunch of utility routines for control/setup packets.
 *   "This file provides the IO requests APIs for control endpoints."
 */

/**
* @brief  USBD_CtlSendData
*         send data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
USBD_Status
USBD_CtlSendData (USB_OTG_CORE_HANDLE  *pdev, 
                               uint8_t *pbuf,
                               uint16_t len)
{
  USBD_Status ret = USBD_OK;
  
  pdev->dev.in_ep[0].total_data_len = len;
  pdev->dev.in_ep[0].rem_data_len   = len;
  pdev->dev.device_state = USB_OTG_EP0_DATA_IN;

  usb_dump ( DM_ENUM, "Tx ctrl", pbuf, len );

  DCD_EP_Tx (pdev, 0, pbuf, len);
 
  return ret;
}

/**
* @brief  USBD_CtlContinueSendData
*         continue sending data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
USBD_Status
USBD_CtlContinueSendData (USB_OTG_CORE_HANDLE  *pdev, 
                                       uint8_t *pbuf,
                                       uint16_t len)
{
  USBD_Status ret = USBD_OK;
  
  DCD_EP_Tx (pdev, 0, pbuf, len);
  
  
  return ret;
}

/**
* @brief  USBD_CtlPrepareRx
*         receive data on the ctl pipe
* @param  pdev: USB OTG device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
USBD_Status
USBD_CtlPrepareRx (USB_OTG_CORE_HANDLE  *pdev,
                                  uint8_t *pbuf,                                  
                                  uint16_t len)
{
  USBD_Status ret = USBD_OK;
  
  pdev->dev.out_ep[0].total_data_len = len;
  pdev->dev.out_ep[0].rem_data_len   = len;
  pdev->dev.device_state = USB_OTG_EP0_DATA_OUT;
  
  DCD_EP_PrepareRx (pdev,
                    0,
                    pbuf,
                    len);
  

  return ret;
}

/**
* @brief  USBD_CtlContinueRx
*         continue receive data on the ctl pipe
* @param  pdev: USB OTG device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
USBD_Status
USBD_CtlContinueRx (USB_OTG_CORE_HANDLE  *pdev, 
                                          uint8_t *pbuf,                                          
                                          uint16_t len)
{
  USBD_Status ret = USBD_OK;
  
  DCD_EP_PrepareRx (pdev,
                    0,                     
                    pbuf,                         
                    len);
  return ret;
}

/**
* @brief  USBD_CtlSendStatus
*         send zero length packet on the ctl pipe
* @param  pdev: USB OTG device instance
* @retval status
*/
USBD_Status
USBD_CtlSendStatus (USB_OTG_CORE_HANDLE  *pdev)
{
  USBD_Status ret = USBD_OK;
  pdev->dev.device_state = USB_OTG_EP0_STATUS_IN;
  DCD_EP_Tx (pdev,
             0,
             NULL, 
             0); 
  
  USB_OTG_EP0_OutStart(pdev);  
  
  return ret;
}

/**
* @brief  USBD_CtlReceiveStatus
*         receive zero length packet on the ctl pipe
* @param  pdev: USB OTG device instance
* @retval status
*/
USBD_Status
USBD_CtlReceiveStatus (USB_OTG_CORE_HANDLE  *pdev)
{
  USBD_Status ret = USBD_OK;
  pdev->dev.device_state = USB_OTG_EP0_STATUS_OUT;  
  DCD_EP_PrepareRx ( pdev,
                    0,
                    NULL,
                    0);  

  USB_OTG_EP0_OutStart(pdev);
  
  return ret;
}

/* ################################################################## */
/* ################################################################## */
/* ################################################################## */

/* Was usbd_req.c
 * -- more routines for control/setup packets.
 *   "This file provides the standard USB requests following chapter 9."
 */

/* 3 variables -- shouldn't these go someplace else?
 * Maybe not, I make them static and nobody complained.
 * (even when this still was usbd_req.c)
 * These should be eradicated.
 * They boil down to local scratch variables
 *  (but note the alignment business)
 */
__ALIGN_BEGIN static uint32_t  USBD_ep_status __ALIGN_END  = 0; 
__ALIGN_BEGIN static uint32_t  USBD_default_cfg __ALIGN_END  = 0;
__ALIGN_BEGIN static uint32_t  USBD_cfg_status __ALIGN_END  = 0;  

static void USBD_GetDescriptor(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_SetAddress(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_SetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_GetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_GetStatus(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_SetFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
static void USBD_ClrFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
// static uint8_t USBD_GetLen(uint8_t *buf);

/**
* @brief  USBD_StdDevReq
*         Handle standard usb device requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
USBD_Status
USBD_StdDevReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  USBD_Status ret = USBD_OK;  
  
  switch (req->bRequest) {
  case USB_REQ_GET_DESCRIPTOR: 
    
    USBD_GetDescriptor (pdev, req);
    break;
    
  case USB_REQ_SET_ADDRESS:                      
    USBD_SetAddress(pdev, req);
    break;
    
  case USB_REQ_SET_CONFIGURATION:                    
    USBD_SetConfig (pdev , req);
    break;
    
  case USB_REQ_GET_CONFIGURATION:                 
    USBD_GetConfig (pdev , req);
    break;
    
  case USB_REQ_GET_STATUS:                                  
    USBD_GetStatus (pdev , req);
    break;
    
    
  case USB_REQ_SET_FEATURE:   
    USBD_SetFeature (pdev , req);    
    break;
    
  case USB_REQ_CLEAR_FEATURE:                                   
    USBD_ClrFeature (pdev , req);
    break;
    
  default:  
    USBD_CtlError(pdev , req);
    break;
  }
  
  return ret;
}

/**
* @brief  USBD_StdItfReq
*         Handle standard usb interface requests
* @param  pdev: USB OTG device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
USBD_Status
USBD_StdItfReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  USBD_Status ret = USBD_OK; 
  
  switch (pdev->dev.device_status) {
  case USB_OTG_CONFIGURED:
    
    if (LOBYTE(req->wIndex) <= USBD_ITF_MAX_NUM) {
      // pdev->dev.class_cb->Setup (pdev, req); 
      CLASS_Setup (pdev, req); 
      
      if((req->wLength == 0)&& (ret == USBD_OK)) {
         USBD_CtlSendStatus(pdev);
      }
    } else {                                               
       USBD_CtlError(pdev , req);
    }
    break;
    
  default:
     USBD_CtlError(pdev , req);
    break;
  }
  return ret;
}

/**
* @brief  USBD_StdEPReq
*         Handle standard usb endpoint requests
* @param  pdev: USB OTG device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
USBD_Status
USBD_StdEPReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  
  uint8_t   ep_addr;
  USBD_Status ret = USBD_OK; 
  
  ep_addr  = LOBYTE(req->wIndex);   
  
  switch (req->bRequest) {
    
  case USB_REQ_SET_FEATURE :
    
    switch (pdev->dev.device_status) {
    case USB_OTG_ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        DCD_EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case USB_OTG_CONFIGURED:   
      if (req->wValue == USB_FEATURE_EP_HALT) {
        if ((ep_addr != 0x00) && (ep_addr != 0x80)) { 
          DCD_EP_Stall(pdev , ep_addr);
        }
      }

      // pdev->dev.class_cb->Setup (pdev, req);   
      CLASS_Setup (pdev, req);   
      USBD_CtlSendStatus(pdev);
      
      break;
      
    default:                         
      USBD_CtlError(pdev , req);
      break;    
    }
    break;
    
  case USB_REQ_CLEAR_FEATURE :
    
    switch (pdev->dev.device_status) {
    case USB_OTG_ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        DCD_EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case USB_OTG_CONFIGURED:   
      if (req->wValue == USB_FEATURE_EP_HALT) {
        if ((ep_addr != 0x00) && (ep_addr != 0x80)) {        
          DCD_EP_ClrStall(pdev , ep_addr);
          // pdev->dev.class_cb->Setup (pdev, req);
          CLASS_Setup (pdev, req);
        }
        USBD_CtlSendStatus(pdev);
      }
      break;
      
    default:                         
       USBD_CtlError(pdev , req);
      break;    
    }
    break;
    
  case USB_REQ_GET_STATUS:                  
    switch (pdev->dev.device_status) {
    case USB_OTG_ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        DCD_EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case USB_OTG_CONFIGURED:         
      
      if ((ep_addr & 0x80)== 0x80) {
        if(pdev->dev.in_ep[ep_addr & 0x7F].is_stall) {
          USBD_ep_status = 0x0001;     
        } else {
          USBD_ep_status = 0x0000;  
        }
      } else if ((ep_addr & 0x80)== 0x00) {
        if(pdev->dev.out_ep[ep_addr].is_stall) {
          USBD_ep_status = 0x0001;     
        } else {
          USBD_ep_status = 0x0000;     
        }      
      }
      USBD_CtlSendData (pdev, (uint8_t *)&USBD_ep_status, 2);
      break;
      
    default:                         
       USBD_CtlError(pdev , req);
      break;
    }
    break;
    
  default:
    break;
  }
  return ret;
}

/**
* @brief  USBD_GetDescriptor
*         Handle Get Descriptor requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/

static void
USBD_GetDescriptor(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  uint16_t len;
  uint8_t *pbuf;
  int stat;

  /* Forward this to the class itself */
  stat = class_get_descriptor ( req->wValue >> 8, pdev, req, &pbuf, &len );

  if ( stat == 0 ) {
	 usb_debug ( DM_DESC, "Get descriptor - FAIL\n" );
     USBD_CtlError(pdev , req);
     return;
  }

  usb_debug ( DM_DESC, "Get descriptor - %d\n", len );
  if ( (len != 0) && (req->wLength != 0) ) {
    len = MIN(len , req->wLength);
	usb_dump ( DM_DESC, "desc:", pbuf, len );
    USBD_CtlSendData (pdev, pbuf, len);
  }
}

/**
* @brief  USBD_SetAddress
*         Set device address
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
USBD_SetAddress(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  uint8_t  dev_addr; 
  
  if ((req->wIndex == 0) && (req->wLength == 0)) {
    dev_addr = (uint8_t)(req->wValue) & 0x7F;     
    
    if (pdev->dev.device_status == USB_OTG_CONFIGURED) {
      USBD_CtlError(pdev , req);
    } else {
      pdev->dev.device_address = dev_addr;
      DCD_EP_SetAddress(pdev, dev_addr);               
      USBD_CtlSendStatus(pdev);                         
      
      if (dev_addr != 0) {
        pdev->dev.device_status  = USB_OTG_ADDRESSED;
      } else {
        pdev->dev.device_status  = USB_OTG_DEFAULT; 
      }
    }
  } else {
     USBD_CtlError(pdev , req);                        
  } 
}

/**
* @brief  USBD_SetConfig
*         Handle Set device configuration request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
USBD_SetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  static uint8_t  cfgidx;
  
  cfgidx = (uint8_t)(req->wValue);                 
  
  if (cfgidx > USBD_CFG_MAX_NUM ) {            
     USBD_CtlError(pdev , req);                              
  } else {
    switch (pdev->dev.device_status) {
    case USB_OTG_ADDRESSED:
	  /* We do see this at the end of enumeration */
	  usb_debug ( DM_ENUM, "Configured %d-- addressed\n", cfgidx );
      if (cfgidx) {                                			   							   							   				
        pdev->dev.device_config = cfgidx;
		/* XXX swapped from below ? */
        pdev->dev.device_status = USB_OTG_CONFIGURED;
        USBD_SetCfg(pdev , cfgidx);
        USBD_CtlSendStatus(pdev);
      } else {
         USBD_CtlSendStatus(pdev);
      }
      break;
      
    case USB_OTG_CONFIGURED:
	  /* We ain't never seen this */
	  usb_debug ( DM_ENUM, "====   Configured %d-- configured\n", cfgidx );
      if (cfgidx == 0) {                           
		/* XXX swapped from above ? */
        pdev->dev.device_status = USB_OTG_ADDRESSED;
        pdev->dev.device_config = cfgidx;          
        USBD_ClrCfg(pdev , cfgidx);
        USBD_CtlSendStatus(pdev);
      } else  if (cfgidx != pdev->dev.device_config) {
        /* Clear old configuration */
        USBD_ClrCfg(pdev , pdev->dev.device_config);
        
        /* set new configuration */
        pdev->dev.device_config = cfgidx;
        USBD_SetCfg(pdev , cfgidx);
        USBD_CtlSendStatus(pdev);
      } else {
        USBD_CtlSendStatus(pdev);
      }
      break;
      
    default:					
       USBD_CtlError(pdev , req);                     
      break;
    }
  }
}

/**
* @brief  USBD_GetConfig
*         Handle Get device configuration request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
USBD_GetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
 
  if (req->wLength != 1) {                   
     USBD_CtlError(pdev , req);
  } else {
    switch (pdev->dev.device_status )  {
    case USB_OTG_ADDRESSED:                     
	  /* XXX this value is never defineed */
      USBD_CtlSendData (pdev, (uint8_t *)&USBD_default_cfg, 1);
      break;
      
    case USB_OTG_CONFIGURED:                   
      USBD_CtlSendData (pdev, &pdev->dev.device_config, 1);
      break;
      
    default:
       USBD_CtlError(pdev , req);
      break;
    }
  }
}

/**
* @brief  USBD_GetStatus
*         Handle Get Status request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
USBD_GetStatus(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  
  switch (pdev->dev.device_status) {
  case USB_OTG_ADDRESSED:
  case USB_OTG_CONFIGURED:
    
    if (pdev->dev.DevRemoteWakeup) {
      USBD_cfg_status = USB_CONFIG_SELF_POWERED | USB_CONFIG_REMOTE_WAKEUP;                                
    } else {
      USBD_cfg_status = USB_CONFIG_SELF_POWERED;   
    }
    
    USBD_CtlSendData (pdev, (uint8_t *)&USBD_cfg_status, 1);
    break;
    
  default :
    USBD_CtlError(pdev , req);
    break;
  }
}

#ifdef notdef
void
HW_SetFeature ( USB_OTG_CORE_HANDLE  *pdev, uint8_t test_mode )
{
    USB_OTG_DCTL_TypeDef     dctl;

    dctl.d32 = USB_OTG_READ_REG32(&pdev->hw->DREGS->DCTL);

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

    USB_OTG_WRITE_REG32(&pdev->hw->DREGS->DCTL, dctl.d32);
}
#endif

/**
* @brief  USBD_SetFeature
*         Handle Set device feature request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*
* tjt - this used to directly access the Dctl register,
*  but we moved that to the driver via the call to
*    HW_SetFeature ()  4-6-2025
*/
static void
USBD_SetFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
    pdev->dev.DevRemoteWakeup = 1;  
    CLASS_Setup (pdev, req);   
    USBD_CtlSendStatus(pdev);
  } else if ((req->wValue == USB_FEATURE_TEST_MODE) && ((req->wIndex & 0xFF) == 0)) {
	HW_SetFeature ( pdev, req->wIndex >> 8 );
    USBD_CtlSendStatus(pdev);
  }
}

/**
* @brief  USBD_ClrFeature
*         Handle clear device feature request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
USBD_ClrFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  switch (pdev->dev.device_status) {
  case USB_OTG_ADDRESSED:
  case USB_OTG_CONFIGURED:
    if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
      pdev->dev.DevRemoteWakeup = 0; 
      // pdev->dev.class_cb->Setup (pdev, req);   
      CLASS_Setup (pdev, req);   
      USBD_CtlSendStatus(pdev);
    }
    break;
    
  default :
     USBD_CtlError(pdev , req);
    break;
  }
}

/**
* @brief  USBD_ParseSetupRequest 
*         Copy buffer into setup structure
* @param  pdev: device instance
* @param  req: usb request
* @retval None
*/
/* EXT */
void
USBD_ParseSetupRequest( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  req->bmRequest     = *(uint8_t *)  (pdev->dev.setup_packet);
  req->bRequest      = *(uint8_t *)  (pdev->dev.setup_packet +  1);
  req->wValue        = SWAPBYTE      (pdev->dev.setup_packet +  2);
  req->wIndex        = SWAPBYTE      (pdev->dev.setup_packet +  4);
  req->wLength       = SWAPBYTE      (pdev->dev.setup_packet +  6);
  
  pdev->dev.in_ep[0].ctl_data_len = req->wLength  ;
  pdev->dev.device_state = USB_OTG_EP0_SETUP;
}

/**
* @brief  USBD_CtlError 
*         Handle USB low level Error
* @param  pdev: device instance
* @param  req: usb request
* @retval None
*/
/* EXT */

void
USBD_CtlError( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
  if((req->bmRequest & 0x80) == 0x80) {
    DCD_EP_Stall(pdev , 0x80);
  } else {
    if(req->wLength == 0) {
       DCD_EP_Stall(pdev , 0x80);
    } else {
      DCD_EP_Stall(pdev , 0);
    }
  }
  USB_OTG_EP0_OutStart(pdev);  
}

/* THE END */
