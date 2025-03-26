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

#include "hydra_usb.h"

#include "usbd_core.h"
#include "usbd_req.h"
#include "usbd_ioreq.h"

#include "driver/usb_dcd_int.h"
#include "driver/usb_dcd.h"

static uint8_t USBD_DataOutStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum);
static uint8_t USBD_DataInStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum);
static uint8_t USBD_SetupStage(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_SOF(USB_OTG_CORE_HANDLE  *pdev);
static uint8_t USBD_Reset(USB_OTG_CORE_HANDLE  *pdev);
static uint8_t USBD_Suspend(USB_OTG_CORE_HANDLE  *pdev);
static uint8_t USBD_Resume(USB_OTG_CORE_HANDLE  *pdev);

static uint8_t USBD_IsoINIncomplete(USB_OTG_CORE_HANDLE  *pdev);
static uint8_t USBD_IsoOUTIncomplete(USB_OTG_CORE_HANDLE  *pdev);
#ifdef VBUS_SENSING_ENABLED
static uint8_t USBD_DevConnected(USB_OTG_CORE_HANDLE  *pdev);
static uint8_t USBD_DevDisconnected(USB_OTG_CORE_HANDLE  *pdev);
#endif

USBD_DCD_INT_cb_TypeDef USBD_DCD_INT_cb = 
{
  USBD_DataOutStage,
  USBD_DataInStage,
  USBD_SetupStage,
  USBD_SOF,
  USBD_Reset,
  USBD_Suspend,
  USBD_Resume,
  USBD_IsoINIncomplete,
  USBD_IsoOUTIncomplete,
#ifdef VBUS_SENSING_ENABLED
  USBD_DevConnected,
  USBD_DevDisconnected,
#endif  
};

USBD_DCD_INT_cb_TypeDef  *USBD_DCD_INT_fops = &USBD_DCD_INT_cb;

/* These macros and functions replace USR_cb and all the stuff
 * in the file usbd_usr.c
 */
static volatile uint8_t usb_status;

#define ST_CONFIGURED 	0x01
#define ST_CONNECTED 	0x02
#define ST_RESUMED 		0x04

#define STATUS_Set(bit)		(usb_status |= bit)
#define STATUS_Clear(bit)	(usb_status &= ~bit)
#define STATUS_Reset()		(usb_status = 0);

/* Accessor functions for the status */

uint8_t usb_isConfigured(void) { return ( usb_status & ST_CONFIGURED ); }

#ifdef VBUS_SENSING_ENABLED
uint8_t usb_isConnected(void) { return ( usb_status & ST_CONNECTED ); }
#else
uint8_t usb_isConnected(void) { return ST_CONNECTED; }
#endif

/**
* @brief  USBD_Init
*         Initialize the device stack and load the class driver
* @param  pdev: device instance
* @param  core_address: USB OTG core ID
*/
void
USBD_Init(USB_OTG_CORE_HANDLE *pdev, USB_OTG_CORE_ID_TypeDef coreID )
               // USBD_DEVICE *pDevice,                  
               // USBD_Class_cb_TypeDef *class_cb, 
               // USBD_Usr_cb_TypeDef *usr_cb)
{

#ifndef HYDRA
  /* No harm to call this, but it does nothing */
  USBD_DeInit(pdev);
#endif
  
  /* Register class and user callbacks */
  /* XXX most of these are gone now */
  pdev->dev.class_cb = NULL;
  pdev->dev.usr_device = NULL;    
  // pdev->dev.usr_cb = usr_cb;  
  // pdev->dev.usr_cb = &USR_cb;
  
  /* set USB OTG core params */
  DCD_Init(pdev , coreID);
  
  /* Upon Init call usr callback */
  // pdev->dev.usr_cb->Init();
  STATUS_Reset();
  
#ifndef HYDRA
  /* Enable Interrupts */
  USB_OTG_BSP_EnableInterrupt(pdev);
#endif
}

/**
* @brief  USBD_DeInit 
*         Re-Initialize the device library
* @param  pdev: device instance
* @retval status: status
*/
USBD_Status USBD_DeInit(USB_OTG_CORE_HANDLE *pdev)
{
  return USBD_OK;
}

USBD_Status USBD_DeInitFull(USB_OTG_CORE_HANDLE *pdev)
{
  /* Software Init */
#ifndef HYDRA
  USB_OTG_BSP_DisableInterrupt(pdev);
  USB_OTG_BSP_DeInit(pdev);
#endif
  
  return USBD_OK;
}

/**
* @brief  USBD_SetupStage 
*         Handle the setup stage
* @param  pdev: device instance
* @retval status
*/
static uint8_t USBD_SetupStage(USB_OTG_CORE_HANDLE *pdev)
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
static uint8_t USBD_DataOutStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum)
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
static uint8_t
USBD_DataInStage(USB_OTG_CORE_HANDLE *pdev , uint8_t epnum)
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

static uint8_t
USBD_Reset(USB_OTG_CORE_HANDLE  *pdev)
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
  
  return USBD_OK;
}

static uint8_t
USBD_Resume(USB_OTG_CORE_HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceResumed(); 
  STATUS_Set ( ST_RESUMED );
  pdev->dev.device_status = USB_OTG_CONFIGURED;  
  return USBD_OK;
}

static uint8_t
USBD_Suspend(USB_OTG_CORE_HANDLE  *pdev)
{
  /* Upon Resume call usr call back */
  // pdev->dev.usr_cb->DeviceSuspended(); 
  STATUS_Clear ( ST_RESUMED );
  pdev->dev.device_status  = USB_OTG_SUSPENDED;
  return USBD_OK;
}

static uint8_t
USBD_SOF(USB_OTG_CORE_HANDLE  *pdev)
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
  return USBD_OK; 
}

USBD_Status
USBD_ClrCfg(USB_OTG_CORE_HANDLE  *pdev, uint8_t cfgidx)
{
  // pdev->dev.class_cb->DeInit(pdev, cfgidx);   
  CLASS_DeInit(pdev, cfgidx);   
  return USBD_OK;
}

static uint8_t
USBD_IsoINIncomplete(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoINIncomplete(pdev);   
  CLASS_IsoINIncomplete ( pdev );   
  return USBD_OK;
}

static uint8_t
USBD_IsoOUTIncomplete(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.class_cb->IsoOUTIncomplete(pdev);   
  CLASS_IsoOUTIncomplete ( pdev );   
  return USBD_OK;
}

#ifdef VBUS_SENSING_ENABLED
static uint8_t
USBD_DevConnected(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceConnected();
  STATUS_Set ( ST_CONNECTED );
  return USBD_OK;
}

static uint8_t
USBD_DevDisconnected(USB_OTG_CORE_HANDLE  *pdev)
{
  // pdev->dev.usr_cb->DeviceDisconnected();
  STATUS_Clear ( ST_CONNECTED );
  // pdev->dev.class_cb->DeInit(pdev, 0);
  CLASS_DeInit(pdev, 0);
  return USBD_OK;
}
#endif

/* THE END */
