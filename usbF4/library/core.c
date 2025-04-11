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
 *   pdev->dev.device_status = DEFAULT;
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
BOGUS_Init ( HANDLE *pdev, CORE_ID_TypeDef coreID )
{
  /* No harm to call this, but it does nothing */
  // DeInit(pdev);
  
  /* set USB OTG core params */
  Init(pdev , coreID);
  
  /* Upon Init call usr callback */
  /* XXX should clean this up */
  STATUS_Reset();
}

Status
BOGUS_DeInit ( HANDLE *pdev )
{
  return OK;
}

Status
DeInitFull ( HANDLE *pdev )
{
  /* Software Init */
  return OK;
}

/* ################################################################## */
/* ################################################################## */
/* ################################################################## */

/**
* @brief  SetupStage 
*         Handle the setup stage
* @param  pdev: device instance
* @retval status
*/
uint8_t
CORE_SetupStage(HANDLE *pdev)
{
  USB_SETUP_REQ req;
  
  ParseSetupRequest(pdev , &req);
  
  switch (req.bmRequest & 0x1F) 
  {
  case USB_REQ_RECIPIENT_DEVICE:   
	usb_debug ( DM_ENUM, "Setup device request\n" );
    StdDevReq (pdev, &req);
    break;
    
  case USB_REQ_RECIPIENT_INTERFACE:     
	usb_debug ( DM_ENUM, "Setup interface request\n" );
    StdItfReq(pdev, &req);
    break;
    
  case USB_REQ_RECIPIENT_ENDPOINT:        
	usb_debug ( DM_ENUM, "Setup Endpoint request\n" );
    StdEPReq(pdev, &req);   
    break;
    
  default:           
	usb_debug ( DM_ENUM, "Setup stall\n" );
    EP_Stall(pdev , req.bmRequest & 0x80);
    break;
  }  
  return OK;
}

/**
* @brief  DataOutStage 
*         Handle data out stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
// static uint8_t
// DataOutStage(HANDLE *pdev , uint8_t epnum)
uint8_t
CORE_DataOutStage(HANDLE *pdev , uint8_t epnum)
{
  EP *ep;
  
  // printf ( "DataOutStage, epnum = %d\n", epnum );

  if(epnum == 0) {
    ep = &pdev->dev.out_ep[0];
    if ( pdev->dev.device_state == EP0_DATA_OUT) {
      if(ep->rem_data_len > ep->maxpacket) {
        ep->rem_data_len -=  ep->maxpacket;
        
        if(pdev->cfg.dma_enable == 1) {
          /* in slave mode this, is handled by the RxStatusQLvl ISR */
          ep->xfer_buff += ep->maxpacket; 
        }        
        CtlContinueRx (pdev, 
                            ep->xfer_buff,
                            MIN(ep->rem_data_len ,ep->maxpacket));
      } else {
        // if((pdev->dev.class_cb->EP0_RxReady != NULL)&&
        //    (pdev->dev.device_status == CONFIGURED)) {
        if ( pdev->dev.device_status == CONFIGURED ) {
          // pdev->dev.class_cb->EP0_RxReady(pdev); 
          CLASS_EP0_RxReady(pdev); 
        }
        CtlSendStatus(pdev);
      }
    }
  }
  // else if((pdev->dev.class_cb->DataOut != NULL)&&
  //         (pdev->dev.device_status == CONFIGURED)) {
  else if (pdev->dev.device_status == CONFIGURED) {
    // pdev->dev.class_cb->DataOut(pdev, epnum); 
    CLASS_DataOut(pdev, epnum); 
  }
  return OK;
}

/**
* @brief  DataInStage 
*         Handle data in stage
* @param  pdev: device instance
* @param  epnum: endpoint index
* @retval status
*/
uint8_t
CORE_DataInStage(HANDLE *pdev , uint8_t epnum)
{
  EP *ep;
  
  if(epnum == 0) 
  {
    ep = &pdev->dev.in_ep[0];
    if ( pdev->dev.device_state == EP0_DATA_IN)
    {
      if(ep->rem_data_len > ep->maxpacket)
      {
        ep->rem_data_len -=  ep->maxpacket;
        if(pdev->cfg.dma_enable == 1)
        {
          /* in slave mode this, is handled by the TxFifoEmpty ISR */
          ep->xfer_buff += ep->maxpacket;
        }
        CtlContinueSendData (pdev, 
                                  ep->xfer_buff, 
                                  ep->rem_data_len);
      }
      else
      { /* last packet is MPS multiple, so send ZLP packet */
        if((ep->total_data_len % ep->maxpacket == 0) &&
           (ep->total_data_len >= ep->maxpacket) &&
             (ep->total_data_len < ep->ctl_data_len )) {
          
          CtlContinueSendData(pdev , NULL, 0);
          ep->ctl_data_len = 0;
        } else {
          // if((pdev->dev.class_cb->EP0_TxSent != NULL)&&
          //    (pdev->dev.device_status == CONFIGURED)) {
          if ( pdev->dev.device_status == CONFIGURED ) {
            // pdev->dev.class_cb->EP0_TxSent(pdev); 
            CLASS_EP0_TxSent ( pdev ); 
          }          
          CtlReceiveStatus(pdev);
        }
      }
    }
  }
  // else if((pdev->dev.class_cb->DataIn != NULL)&& 
  //         (pdev->dev.device_status == CONFIGURED)) {
  else if (pdev->dev.device_status == CONFIGURED) {
    // pdev->dev.class_cb->DataIn(pdev, epnum); 
    CLASS_DataIn(pdev, epnum); 
  }
  return OK;
}

uint8_t
CORE_Reset(HANDLE  *pdev)
{
  /* Open EP0 OUT */
  EP_Open(pdev,
              0x00,
              MAX_EP0_SIZE,
              EP_TYPE_CTRL);
  
  /* Open EP0 IN */
  EP_Open(pdev,
              0x80,
              MAX_EP0_SIZE,
              EP_TYPE_CTRL);
  
  /* Upon Reset call usr call back */
  pdev->dev.device_status = DEFAULT;
  // pdev->dev.usr_cb->DeviceReset(pdev->cfg.speed);
  STATUS_Reset();
  usb_debug ( DM_EVENT, "Reset\n" );
  
  return OK;
}

uint8_t
CORE_Resume(HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceResumed(); 
  STATUS_Set ( ST_RESUMED );
  usb_debug ( DM_EVENT, "Resumed\n" );
  pdev->dev.device_status = CONFIGURED;  
  return OK;
}

uint8_t
CORE_Suspend(HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceSuspended(); 
  STATUS_Clear ( ST_RESUMED );
  usb_debug ( DM_EVENT, "Suspended\n" );
  pdev->dev.device_status  = SUSPENDED;
  return OK;
}

uint8_t
CORE_SOF(HANDLE  *pdev)
{
  // if(pdev->dev.class_cb->SOF) {
  //   pdev->dev.class_cb->SOF(pdev); 
  // }
  CLASS_SOF(pdev); 
  return OK;
}

Status
SetCfg(HANDLE  *pdev, uint8_t cfgidx)
{
  // pdev->dev.class_cb->Init(pdev, cfgidx); 
  CLASS_Init(pdev, cfgidx); 
  
  /* Upon set config call usr call back */
  // pdev->dev.usr_cb->DeviceConfigured();
  STATUS_Set ( ST_CONFIGURED );
  usb_debug ( DM_EVENT, "Configured\n" );
  return OK; 
}

Status
ClrCfg(HANDLE  *pdev, uint8_t cfgidx)
{
  // pdev->dev.class_cb->DeInit(pdev, cfgidx);   
  CLASS_DeInit(pdev, cfgidx);   
  return OK;
}

 uint8_t
CORE_IsoINIncomplete(HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoINIncomplete(pdev);   
  CLASS_IsoINIncomplete ( pdev );   
  return OK;
}

uint8_t
CORE_IsoOUTIncomplete(HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoOUTIncomplete(pdev);   
  CLASS_IsoOUTIncomplete ( pdev );   
  return OK;
}

#ifdef VBUS_SENSING_ENABLED
uint8_t
CORE_DevConnected(HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceConnected();
  STATUS_Set ( ST_CONNECTED );
  usb_debug ( DM_EVENT, "Connected\n" );
  return OK;
}

uint8_t
CORE_DevDisconnected(HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceDisconnected();
  STATUS_Clear ( ST_CONNECTED );
  usb_debug ( DM_EVENT, "Disconnected\n" );
  // pdev->dev.class_cb->DeInit(pdev, 0);
  CLASS_DeInit(pdev, 0);
  return OK;
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
* @brief  CtlSendData
*         send data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
Status
CtlSendData (HANDLE  *pdev, 
                               uint8_t *pbuf,
                               uint16_t len)
{
  Status ret = OK;
  
  pdev->dev.in_ep[0].total_data_len = len;
  pdev->dev.in_ep[0].rem_data_len   = len;
  pdev->dev.device_state = EP0_DATA_IN;

  usb_dump ( DM_ENUM, "Tx ctrl", pbuf, len );

  EP_Tx (pdev, 0, pbuf, len);
 
  return ret;
}

/**
* @brief  CtlContinueSendData
*         continue sending data on the ctl pipe
* @param  pdev: device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be sent
* @retval status
*/
Status
CtlContinueSendData (HANDLE  *pdev, 
                                       uint8_t *pbuf,
                                       uint16_t len)
{
  Status ret = OK;
  
  EP_Tx (pdev, 0, pbuf, len);
  
  
  return ret;
}

/**
* @brief  CtlPrepareRx
*         receive data on the ctl pipe
* @param  pdev: USB OTG device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
Status
CtlPrepareRx (HANDLE  *pdev,
                                  uint8_t *pbuf,                                  
                                  uint16_t len)
{
  Status ret = OK;
  
  pdev->dev.out_ep[0].total_data_len = len;
  pdev->dev.out_ep[0].rem_data_len   = len;
  pdev->dev.device_state = EP0_DATA_OUT;
  
  EP_PrepareRx (pdev,
                    0,
                    pbuf,
                    len);
  

  return ret;
}

/**
* @brief  CtlContinueRx
*         continue receive data on the ctl pipe
* @param  pdev: USB OTG device instance
* @param  buff: pointer to data buffer
* @param  len: length of data to be received
* @retval status
*/
Status
CtlContinueRx (HANDLE  *pdev, 
                                          uint8_t *pbuf,                                          
                                          uint16_t len)
{
  Status ret = OK;
  
  EP_PrepareRx (pdev,
                    0,                     
                    pbuf,                         
                    len);
  return ret;
}

/**
* @brief  CtlSendStatus
*         send zero length packet on the ctl pipe
* @param  pdev: USB OTG device instance
* @retval status
*/
Status
CtlSendStatus (HANDLE  *pdev)
{
  Status ret = OK;

  pdev->dev.device_state = EP0_STATUS_IN;

  usb_debug ( DM_EVENT, "zero length status\n" );
  EP_Tx (pdev,
             0,
             NULL, 
             0); 
  
  EP0_OutStart(pdev);  
  
  return ret;
}

/**
* @brief  CtlReceiveStatus
*         receive zero length packet on the ctl pipe
* @param  pdev: USB OTG device instance
* @retval status
*/
Status
CtlReceiveStatus (HANDLE  *pdev)
{
  Status ret = OK;
  pdev->dev.device_state = EP0_STATUS_OUT;  
  EP_PrepareRx ( pdev,
                    0,
                    NULL,
                    0);  

  EP0_OutStart(pdev);
  
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
__ALIGN_BEGIN static uint32_t  ep_status __ALIGN_END  = 0; 
__ALIGN_BEGIN static uint32_t  default_cfg __ALIGN_END  = 0;
__ALIGN_BEGIN static uint32_t  cfg_status __ALIGN_END  = 0;  

static void GetDescriptor(HANDLE  *pdev, USB_SETUP_REQ *req);
static void SetAddress(HANDLE  *pdev, USB_SETUP_REQ *req);
static void SetConfig(HANDLE  *pdev, USB_SETUP_REQ *req);
static void GetConfig(HANDLE  *pdev, USB_SETUP_REQ *req);
static void GetStatus(HANDLE  *pdev, USB_SETUP_REQ *req);
static void SetFeature(HANDLE  *pdev, USB_SETUP_REQ *req);
static void ClrFeature(HANDLE  *pdev, USB_SETUP_REQ *req);
// static uint8_t GetLen(uint8_t *buf);

/**
* @brief  StdDevReq
*         Handle standard usb device requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
Status
StdDevReq (HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  Status ret = OK;  
  
  switch (req->bRequest) {
  case USB_REQ_GET_DESCRIPTOR: 
    
    GetDescriptor (pdev, req);
    break;
    
  case USB_REQ_SET_ADDRESS:                      
    SetAddress(pdev, req);
    break;
    
  case USB_REQ_SET_CONFIGURATION:                    
    SetConfig (pdev , req);
    break;
    
  case USB_REQ_GET_CONFIGURATION:                 
    GetConfig (pdev , req);
    break;
    
  case USB_REQ_GET_STATUS:                                  
    GetStatus (pdev , req);
    break;
    
    
  case USB_REQ_SET_FEATURE:   
    SetFeature (pdev , req);    
    break;
    
  case USB_REQ_CLEAR_FEATURE:                                   
    ClrFeature (pdev , req);
    break;
    
  default:  
    CtlError(pdev , req);
    break;
  }
  
  return ret;
}

/**
* @brief  StdItfReq
*         Handle standard usb interface requests
* @param  pdev: USB OTG device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
Status
StdItfReq (HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  Status ret = OK; 
  
  switch (pdev->dev.device_status) {
  case CONFIGURED:
    
    if (LOBYTE(req->wIndex) <= ITF_MAX_NUM) {
      // pdev->dev.class_cb->Setup (pdev, req); 
      CLASS_Setup (pdev, req); 
      
      if((req->wLength == 0)&& (ret == OK)) {
         CtlSendStatus(pdev);
      }
    } else {                                               
       CtlError(pdev , req);
    }
    break;
    
  default:
     CtlError(pdev , req);
    break;
  }
  return ret;
}

/**
* @brief  StdEPReq
*         Handle standard usb endpoint requests
* @param  pdev: USB OTG device instance
* @param  req: usb request
* @retval status
*/
/* EXT */
Status
StdEPReq (HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  
  uint8_t   ep_addr;
  Status ret = OK; 
  
  ep_addr  = LOBYTE(req->wIndex);   
  
  switch (req->bRequest) {
    
  case USB_REQ_SET_FEATURE :
    
    switch (pdev->dev.device_status) {
    case ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case CONFIGURED:   
      if (req->wValue == USB_FEATURE_EP_HALT) {
        if ((ep_addr != 0x00) && (ep_addr != 0x80)) { 
          EP_Stall(pdev , ep_addr);
        }
      }

      // pdev->dev.class_cb->Setup (pdev, req);   
      CLASS_Setup (pdev, req);   
      CtlSendStatus(pdev);
      
      break;
      
    default:                         
      CtlError(pdev , req);
      break;    
    }
    break;
    
  case USB_REQ_CLEAR_FEATURE :
    
    switch (pdev->dev.device_status) {
    case ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case CONFIGURED:   
      if (req->wValue == USB_FEATURE_EP_HALT) {
        if ((ep_addr != 0x00) && (ep_addr != 0x80)) {        
          EP_ClrStall(pdev , ep_addr);
          // pdev->dev.class_cb->Setup (pdev, req);
          CLASS_Setup (pdev, req);
        }
        CtlSendStatus(pdev);
      }
      break;
      
    default:                         
       CtlError(pdev , req);
      break;    
    }
    break;
    
  case USB_REQ_GET_STATUS:                  
    switch (pdev->dev.device_status) {
    case ADDRESSED:          
      if ((ep_addr != 0x00) && (ep_addr != 0x80)) {
        EP_Stall(pdev , ep_addr);
      }
      break;	
      
    case CONFIGURED:         
      
      if ((ep_addr & 0x80)== 0x80) {
        if(pdev->dev.in_ep[ep_addr & 0x7F].is_stall) {
          ep_status = 0x0001;     
        } else {
          ep_status = 0x0000;  
        }
      } else if ((ep_addr & 0x80)== 0x00) {
        if(pdev->dev.out_ep[ep_addr].is_stall) {
          ep_status = 0x0001;     
        } else {
          ep_status = 0x0000;     
        }      
      }
      CtlSendData (pdev, (uint8_t *)&ep_status, 2);
      break;
      
    default:                         
       CtlError(pdev , req);
      break;
    }
    break;
    
  default:
    break;
  }
  return ret;
}

/**
* @brief  GetDescriptor
*         Handle Get Descriptor requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/

static void
GetDescriptor(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  uint16_t len;
  uint8_t *pbuf;
  int stat;

  /* Forward this to the class itself */
  stat = class_get_descriptor ( req->wValue >> 8, pdev, req, &pbuf, &len );

  if ( stat == 0 ) {
	 usb_debug ( DM_DESC, "Get descriptor - FAIL\n" );
     CtlError(pdev , req);
     return;
  }

  usb_debug ( DM_DESC, "Get descriptor - %d\n", len );
  if ( (len != 0) && (req->wLength != 0) ) {
    len = MIN(len , req->wLength);
	usb_dump ( DM_DESC, "desc:", pbuf, len );
    CtlSendData (pdev, pbuf, len);
  }
}

/**
* @brief  SetAddress
*         Set device address
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
SetAddress(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  uint8_t  dev_addr; 
  
  if ((req->wIndex == 0) && (req->wLength == 0)) {
    dev_addr = (uint8_t)(req->wValue) & 0x7F;     
    
    if (pdev->dev.device_status == CONFIGURED) {
      CtlError(pdev , req);
    } else {
      pdev->dev.device_address = dev_addr;
      EP_SetAddress(pdev, dev_addr);               
      CtlSendStatus(pdev);                         
      
      if (dev_addr != 0) {
        pdev->dev.device_status  = ADDRESSED;
      } else {
        pdev->dev.device_status  = DEFAULT; 
      }
    }
  } else {
     CtlError(pdev , req);                        
  } 
}

/**
* @brief  SetConfig
*         Handle Set device configuration request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
SetConfig(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  static uint8_t  cfgidx;
  
  cfgidx = (uint8_t)(req->wValue);                 
  
  if (cfgidx > CFG_MAX_NUM ) {            
     CtlError(pdev , req);                              
  } else {
    switch (pdev->dev.device_status) {
    case ADDRESSED:
	  /* We do see this at the end of enumeration */
	  usb_debug ( DM_ENUM, "Configured %d-- addressed\n", cfgidx );
      if (cfgidx) {                                			   							   							   				
        pdev->dev.device_config = cfgidx;
		/* XXX swapped from below ? */
        pdev->dev.device_status = CONFIGURED;
        SetCfg(pdev , cfgidx);
        CtlSendStatus(pdev);
      } else {
         CtlSendStatus(pdev);
      }
      break;
      
    case CONFIGURED:
	  /* We ain't never seen this */
	  usb_debug ( DM_ENUM, "====   Configured %d-- configured\n", cfgidx );
      if (cfgidx == 0) {                           
		/* XXX swapped from above ? */
        pdev->dev.device_status = ADDRESSED;
        pdev->dev.device_config = cfgidx;          
        ClrCfg(pdev , cfgidx);
        CtlSendStatus(pdev);
      } else  if (cfgidx != pdev->dev.device_config) {
        /* Clear old configuration */
        ClrCfg(pdev , pdev->dev.device_config);
        
        /* set new configuration */
        pdev->dev.device_config = cfgidx;
        SetCfg(pdev , cfgidx);
        CtlSendStatus(pdev);
      } else {
        CtlSendStatus(pdev);
      }
      break;
      
    default:					
       CtlError(pdev , req);                     
      break;
    }
  }
}

/**
* @brief  GetConfig
*         Handle Get device configuration request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
GetConfig(HANDLE  *pdev, USB_SETUP_REQ *req)
{
 
  if (req->wLength != 1) {                   
     CtlError(pdev , req);
  } else {
    switch (pdev->dev.device_status )  {
    case ADDRESSED:                     
	  /* XXX this value is never defineed */
      CtlSendData (pdev, (uint8_t *)&default_cfg, 1);
      break;
      
    case CONFIGURED:                   
      CtlSendData (pdev, &pdev->dev.device_config, 1);
      break;
      
    default:
       CtlError(pdev , req);
      break;
    }
  }
}

/**
* @brief  GetStatus
*         Handle Get Status request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
GetStatus(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  
  switch (pdev->dev.device_status) {
  case ADDRESSED:
  case CONFIGURED:
    
    if (pdev->dev.DevRemoteWakeup) {
      cfg_status = USB_CONFIG_SELF_POWERED | USB_CONFIG_REMOTE_WAKEUP;                                
    } else {
      cfg_status = USB_CONFIG_SELF_POWERED;   
    }
    
    CtlSendData (pdev, (uint8_t *)&cfg_status, 1);
    break;
    
  default :
    CtlError(pdev , req);
    break;
  }
}

#ifdef notdef
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
#endif

/**
* @brief  SetFeature
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
SetFeature(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
    pdev->dev.DevRemoteWakeup = 1;  
    CLASS_Setup (pdev, req);   
    CtlSendStatus(pdev);
  } else if ((req->wValue == USB_FEATURE_TEST_MODE) && ((req->wIndex & 0xFF) == 0)) {
	HW_SetFeature ( pdev, req->wIndex >> 8 );
    CtlSendStatus(pdev);
  }
}

/**
* @brief  ClrFeature
*         Handle clear device feature request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
static void
ClrFeature(HANDLE  *pdev, USB_SETUP_REQ *req)
{
  switch (pdev->dev.device_status) {
  case ADDRESSED:
  case CONFIGURED:
    if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
      pdev->dev.DevRemoteWakeup = 0; 
      // pdev->dev.class_cb->Setup (pdev, req);   
      CLASS_Setup (pdev, req);   
      CtlSendStatus(pdev);
    }
    break;
    
  default :
     CtlError(pdev , req);
    break;
  }
}

/**
* @brief  ParseSetupRequest 
*         Copy buffer into setup structure
* @param  pdev: device instance
* @param  req: usb request
* @retval None
*/
/* EXT */
void
ParseSetupRequest( HANDLE  *pdev, USB_SETUP_REQ *req)
{
  req->bmRequest     = *(uint8_t *)  (pdev->dev.setup_packet);
  req->bRequest      = *(uint8_t *)  (pdev->dev.setup_packet +  1);
  req->wValue        = SWAPBYTE      (pdev->dev.setup_packet +  2);
  req->wIndex        = SWAPBYTE      (pdev->dev.setup_packet +  4);
  req->wLength       = SWAPBYTE      (pdev->dev.setup_packet +  6);
  
  pdev->dev.in_ep[0].ctl_data_len = req->wLength  ;
  pdev->dev.device_state = EP0_SETUP;
}

/**
* @brief  CtlError 
*         Handle USB low level Error
* @param  pdev: device instance
* @param  req: usb request
* @retval None
*/
/* EXT */

void
CtlError( HANDLE  *pdev, USB_SETUP_REQ *req)
{
  if((req->bmRequest & 0x80) == 0x80) {
    EP_Stall(pdev , 0x80);
  } else {
    if(req->wLength == 0) {
       EP_Stall(pdev , 0x80);
    } else {
      EP_Stall(pdev , 0);
    }
  }
  EP0_OutStart(pdev);  
}

/* THE END */
