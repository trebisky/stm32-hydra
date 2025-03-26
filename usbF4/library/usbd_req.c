/**
  ******************************************************************************
  * @file    usbd_req.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011  
  * @brief   This file provides the standard USB requests following chapter 9.
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_req.h"
#include "usbd_ioreq.h"

#include "hydra_usb.h"

__ALIGN_BEGIN uint32_t USBD_ep_status __ALIGN_END  = 0; 

__ALIGN_BEGIN uint32_t  USBD_default_cfg __ALIGN_END  = 0;

__ALIGN_BEGIN uint32_t  USBD_cfg_status __ALIGN_END  = 0;  

static void USBD_GetDescriptor(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_SetAddress(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_SetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_GetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_GetStatus(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_SetFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static void USBD_ClrFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

static uint8_t USBD_GetLen(uint8_t *buf);

/**
* @brief  USBD_StdDevReq
*         Handle standard usb device requests
* @param  pdev: device instance
* @param  req: usb request
* @retval status
*/
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
USBD_Status
USBD_StdItfReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req)
{
  USBD_Status ret = USBD_OK; 
  
  switch (pdev->dev.device_status) {
  case USB_OTG_CONFIGURED:
    
    if (LOBYTE(req->wIndex) <= USBD_ITF_MAX_NUM) {
      // pdev->dev.class_cb->Setup (pdev, req); 
      Klass_Setup (pdev, req); 
      
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
      Klass_Setup (pdev, req);   
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
          Klass_Setup (pdev, req);
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
static void USBD_GetConfig(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{
 
  if (req->wLength != 1) {                   
     USBD_CtlError(pdev , req);
  } else {
    switch (pdev->dev.device_status )  {
    case USB_OTG_ADDRESSED:                     
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


/**
* @brief  USBD_SetFeature
*         Handle Set device feature request
* @param  pdev: device instance
* @param  req: usb request
* @retval status
* XXX - direct access to hardware here .. fix
*/
static void
USBD_SetFeature(USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req)
{

  USB_OTG_DCTL_TypeDef     dctl;
  uint8_t test_mode = 0;
 
  if (req->wValue == USB_FEATURE_REMOTE_WAKEUP) {
    pdev->dev.DevRemoteWakeup = 1;  
    // pdev->dev.class_cb->Setup (pdev, req);   
    Klass_Setup (pdev, req);   
    USBD_CtlSendStatus(pdev);
  } else if ((req->wValue == USB_FEATURE_TEST_MODE) && 
           ((req->wIndex & 0xFF) == 0)) {
    dctl.d32 = USB_OTG_READ_REG32(&pdev->regs.DREGS->DCTL);
    
    test_mode = req->wIndex >> 8;
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

    USB_OTG_WRITE_REG32(&pdev->regs.DREGS->DCTL, dctl.d32);
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
      Klass_Setup (pdev, req);   
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
