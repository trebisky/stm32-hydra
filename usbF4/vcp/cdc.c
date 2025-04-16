/**
  ******************************************************************************
  * @file    usbd_cdc_core.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011
  * @brief   This file provides the high layer firmware functions to manage the 
  *          following functionalities of the USB CDC Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *
  ******************************************************************************               
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */

#include "types.h"
#include "usb_conf.h"

#include "usb_std.h"

#include "usb_core.h"

#include "protos.h"

#include "usb_conf.h"
#include "conf.h"
#include "vcp.h"


/**
  *          ===================================================================      
  *                                CDC Class Driver Description
  *          =================================================================== 
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus 
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class

  *           @note
  *             For the Abstract Control Model, this core allows only transmitting the requests to
  *             lower layer dispatcher (ie. usbd_cdc_vcp.c/.h) which should manage each request and
  *             perform relative actions.
  * 
  *           These aspects may be enriched or modified for a specific user application.
  *          
  *            This driver doesn't implement the following aspects of the specification 
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *      
  */ 


/*********************************************
   prototypes for CDC Device library callbacks
 *********************************************/
// static uint8_t  usbd_cdc_Init        (void  *pdev, uint8_t cfgidx);
// static uint8_t  usbd_cdc_DeInit      (void  *pdev, uint8_t cfgidx);
//static uint8_t  usbd_cdc_Setup       (void  *pdev, USB_SETUP_REQ *req);
// static uint8_t  usbd_cdc_EP0_RxReady  (void *pdev);
// static uint8_t  usbd_cdc_DataIn      (void *pdev, uint8_t epnum);
// static uint8_t  usbd_cdc_DataOut     (void *pdev, uint8_t epnum);
// static uint8_t  usbd_cdc_SOF         (void *pdev);

/*********************************************
   CDC specific management functions
 *********************************************/
static void Handle_USBAsynchXfer  (void *pdev);

__ALIGN_BEGIN static __IO uint32_t  usbd_cdc_AltSet  __ALIGN_END = 0;

__ALIGN_BEGIN uint8_t __CCMRAM__ USB_Rx_Buffer[CDC_DATA_MAX_PACKET_SIZE] __ALIGN_END ;

/* tjt - XXX - it is odd that only the Tx buffer is not specified as
 * being in CCMRAM - and also note that depending on the linker file,
 * it could end up in CCMRAM anyway, this is just a hint to say
 * "put it ih CCMRAM if possible" -- however CCMRAM is supposed to
 * not work with DMA, so there could be trouble waiting here.
 */
#ifdef HS_INTERNAL_DMA_ENABLED
__ALIGN_BEGIN uint8_t APP_Tx_Buffer   [APP_TX_DATA_SIZE] __ALIGN_END ;
#else 
__ALIGN_BEGIN uint8_t __CCMRAM__ APP_Tx_Buffer[APP_TX_DATA_SIZE] __ALIGN_END ;
#endif /* HS_INTERNAL_DMA_ENABLED */
 
__ALIGN_BEGIN uint8_t CmdBuff[CDC_CMD_PACKET_SZE] __ALIGN_END ;

volatile uint16_t APP_Tx_ptr_in  = 0;
volatile uint16_t APP_Tx_ptr_out = 0;

uint8_t  USB_Tx_State = 0;

static uint32_t cdcCmd = 0xFF;
static uint32_t cdcLen = 0;

#ifdef notdef
static uint8_t  *
bogusDesc (uint8_t speed, uint16_t *length)
{
	panic ( "bogusDesc" );
}

/* CDC interface class callbacks structure */
Class_cb_TypeDef  CDC_cb = 
{
  usbd_cdc_Init,
  usbd_cdc_DeInit,
  usbd_cdc_Setup,
  NULL,                 /* EP0_TxSent, */
  usbd_cdc_EP0_RxReady,
  usbd_cdc_DataIn,
  usbd_cdc_DataOut,
  usbd_cdc_SOF,
  NULL,
  NULL,     
#ifdef notdef
  bogusDesc,
#ifdef USE_HS   
  bogusDesc
#endif /* USE_HS  */
#endif
};
#endif

#ifdef notdef
Class_cb_TypeDef  CDC_cb = 
{
  NULL,		/* Init */
  NULL,
  NULL,					/* Setup */
  NULL,                 /* EP0_TxSent */
  NULL,					/* EP0_RxReady */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

/**
  * @brief  usbd_cdc_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
// static uint8_t
// usbd_cdc_Init (void  *pdev, uint8_t cfgidx)
uint8_t
CLASS_Init (void  *pdev, uint8_t cfgidx)
{
  uint8_t *pbuf;

  /* Open EP IN */
  EP_Open(pdev,
              CDC_IN_EP,
              CDC_DATA_IN_PACKET_SIZE,
              EP_BULK);
  
  /* Open EP OUT */
  EP_Open(pdev,
              CDC_OUT_EP,
              CDC_DATA_OUT_PACKET_SIZE,
              EP_BULK);
  
  /* Open Command IN EP */
  EP_Open(pdev,
              CDC_CMD_EP,
              CDC_CMD_PACKET_SZE,
              EP_INT);
 
#ifdef notdef
  pbuf = (uint8_t *) DeviceDesc;
  pbuf[4] = DEVICE_CLASS_CDC;
  pbuf[5] = DEVICE_SUBCLASS_CDC;
#endif
  
  /* Initialize the Interface physical components */
  // APP_FOPS.pIf_Init(pdev);
  VCP_Init(pdev);

  /* Prepare Out endpoint to receive next packet */
  EP_PrepareRx(pdev,
                   CDC_OUT_EP,
                   (uint8_t*)(USB_Rx_Buffer),
                   CDC_DATA_OUT_PACKET_SIZE);
  
  return OK;
}

/**
  * @brief  usbd_cdc_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
// static uint8_t  
// usbd_cdc_DeInit (void  *pdev, uint8_t cfgidx)
uint8_t  
CLASS_DeInit (void  *pdev, uint8_t cfgidx)
{
  /* Open EP IN */
  EP_Close(pdev,
              CDC_IN_EP);
  
  /* Open EP OUT */
  EP_Close(pdev,
              CDC_OUT_EP);
  
  /* Open Command IN EP */
  EP_Close(pdev,
              CDC_CMD_EP);

  /* Restore default state of the Interface physical components */
  // APP_FOPS.pIf_DeInit();
  VCP_DeInit();
  
  return OK;
}

void
CLASS_EP0_TxSent ( void *pdev )
{
	/* nothing */
}

void
CLASS_IsoINIncomplete ( void *pdev )
{
	/* nothing */
}

void
CLASS_IsoOUTIncomplete ( void *pdev )
{
	/* nothing */
}

/**
  * @brief  usbd_cdc_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
// static uint8_t
// usbd_cdc_Setup (void  *pdev, USB_SETUP_REQ *req)
uint8_t
CLASS_Setup (void  *pdev, USB_SETUP_REQ *req)
{
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* CDC Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
      /* Check if the request is a data setup packet */
      if (req->wLength)
      {
        /* Check if the request is Device-to-Host */
        if (req->bmRequest & 0x80)
        {
          /* Get the data to be sent to Host from interface layer */
          // APP_FOPS.pIf_Ctrl(req->bRequest, CmdBuff, req->wLength);
          VCP_Ctrl(req->bRequest, CmdBuff, req->wLength);
          
          /* Send the data to the host */
          CtlSendData (pdev, 
                            CmdBuff,
                            req->wLength);          
        }
        else /* Host-to-Device request */
        {
          /* Set the value of the current command to be processed */
          cdcCmd = req->bRequest;
          cdcLen = req->wLength;
          
          /* Prepare the reception of the buffer over EP0
          Next step: the received data will be managed in usbd_cdc_EP0_TxSent() 
          function. */
          CtlPrepareRx (pdev,
                             CmdBuff,
                             req->wLength);          
        }
      }
      else /* No Data request */
      {
        /* Transfer the command to the interface layer */
        // APP_FOPS.pIf_Ctrl(req->bRequest, (uint8_t*)&req->wValue, sizeof(req->wValue));
        VCP_Ctrl(req->bRequest, (uint8_t*)&req->wValue, sizeof(req->wValue));
      }
      
      return OK;
      
    default:
      CtlError (pdev, req);
      return FAIL;
    
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( (req->wValue >> 8) == CDC_DESCRIPTOR_TYPE) {
        uint8_t  *pbuf;

		panic ( "vcp/usbd_cdc_core CDC descriptor type requested" );

/* We cannot compile with DMA enabled due to usbd_cdc_Desc being
 * absent.  We have had the code for the "panic 1" in place for
 * a long time and never seen this code tickled,
 * so I replace the missing reference with "panic 2" and we will
 * see what happens.  It just works.
 * Then I added the panic call above.
 * This code never gets called.
 */
#ifdef HS_INTERNAL_DMA_ENABLED
		/* XXX - trouble here, where is this? */
		// tjt 4-13-2025
        // pbuf = usbd_cdc_Desc;   
		panic ( "vcp/usbd_cdc_core unhappy get descriptor 2" );
#else
		/* XXX */
		panic ( "vcp/usbd_cdc_core unhappy get descriptor 1" );
        // pbuf = usbd_cdc_CfgDesc + 9 + (9 * ITF_MAX_NUM);
#endif 
        uint16_t len = MIN(USB_CDC_DESC_SIZ , req->wLength);
      
        CtlSendData (pdev, pbuf, len);
      }
      break;
      
    case USB_REQ_GET_INTERFACE :
      CtlSendData (pdev, (uint8_t *)&usbd_cdc_AltSet, 1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < ITF_MAX_NUM) {
        usbd_cdc_AltSet = (uint8_t)(req->wValue);
      } else {
        /* Call the error management function (command will be nacked */
        CtlError (pdev, req);
      }
      break;
    }
  }
  return OK;
}

/**
  * @brief  usbd_cdc_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device instance
  * @retval status
  */
// static uint8_t
// usbd_cdc_EP0_RxReady (void  *pdev)
uint8_t
CLASS_EP0_RxReady ( void  *pdev)
{ 
  if (cdcCmd != NO_CMD) {
    /* Process the data */
    // APP_FOPS.pIf_Ctrl(cdcCmd, CmdBuff, cdcLen);
    VCP_Ctrl(cdcCmd, CmdBuff, cdcLen);
    
    /* Reset the command variable to default value */
    cdcCmd = NO_CMD;
  }
  
  return OK;
}

/**
  * @brief  usbd_audio_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
// ala42: applied fix from
//https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2fpublic%2f
//STe2ecommunities%2fmcu%2fLists%2fcortex_mx_stm32%2fUSB%20CDC%20Device%20hung%20fix&
//FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=75

// static uint8_t
// usbd_cdc_DataIn (void *pdev, uint8_t epnum)
uint8_t
CLASS_DataIn (void *pdev, uint8_t epnum)
{
	if (USB_Tx_State == 0) return OK;

    uint16_t USB_Tx_ptr = APP_Tx_ptr_out;
    uint16_t USB_Tx_length = (APP_Tx_ptr_in - USB_Tx_ptr) & APP_TX_DATA_SIZE_MASK;

    usb_debug ( DM_ORIG, "usbd_cdc_DataIn called with %d bytes waiting to send on endpoint %d\n", USB_Tx_length, epnum );

    if (USB_Tx_length == 0) {
        //USB_Tx_State = 0;
        if (((HANDLE*)pdev)->dev.in_ep[epnum].xfer_len != CDC_DATA_IN_PACKET_SIZE) {
            USB_Tx_State = 0;
            return OK;
        }
        /* Transmit zero sized packet in case the last one has maximum allowed size. Otherwise
         * the recipient may expect more data coming soon and not return buffered data to app.
         * See section 5.8.3 Bulk Transfer Packet Size Constraints
         * of the USB Specification document.
        */
    }

	usb_debug ( DM_WRITE1, "USB Tx datain start: %d\n", USB_Tx_length );

    if (USB_Tx_length > CDC_DATA_IN_PACKET_SIZE) {
       USB_Tx_length = CDC_DATA_IN_PACKET_SIZE;
    }

    if ( USB_Tx_length > (APP_TX_DATA_SIZE - USB_Tx_ptr) ) {
       USB_Tx_length = (APP_TX_DATA_SIZE - USB_Tx_ptr);
    }

    APP_Tx_ptr_out = (USB_Tx_ptr + USB_Tx_length) & APP_TX_DATA_SIZE_MASK;

	usb_debug ( DM_WRITE1, "USB Tx datain send: %d\n", USB_Tx_length );

    /* Prepare the available data buffer to be sent on IN endpoint */
    EP_Tx (pdev,
                 CDC_IN_EP,
                 (uint8_t*)&APP_Tx_Buffer[USB_Tx_ptr],
                 USB_Tx_length);

  return OK;
}

void usbd_cdc_PrepareRx (void *pdev)
{
    EP_PrepareRx(pdev, CDC_OUT_EP, USB_Rx_Buffer, CDC_DATA_OUT_PACKET_SIZE);
}

/**
  * @brief  usbd_cdc_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
// static uint8_t usbd_cdc_DataOut(void *pdev, uint8_t epnum)
uint8_t
CLASS_DataOut(void *pdev, uint8_t epnum)
{      
  /* Get the received data buffer and update the counter */
  uint16_t USB_Rx_Cnt = ((HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;
  
  /* USB data will be immediately processed, this allow next USB traffic being 
     NAKed till the end of the application Xfer */
  // if ( APP_FOPS.pIf_DataRx(USB_Rx_Buffer, USB_Rx_Cnt)==OK ) {
  if ( VCP_DataRx(USB_Rx_Buffer, USB_Rx_Cnt)==OK ) {
    /* Prepare Out endpoint to receive next packet */
    EP_PrepareRx(pdev, CDC_OUT_EP, USB_Rx_Buffer, CDC_DATA_OUT_PACKET_SIZE);
  }
  return OK;
}

/**
  * @brief  usbd_cdc_SOF
  *         Start Of Frame event management
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
// static uint8_t  usbd_cdc_SOF(void *pdev)
uint8_t
CLASS_SOF(void *pdev)
{      
  static uint32_t FrameCount = 0;
  
  if (FrameCount++ == CDC_IN_FRAME_INTERVAL) {
    /* Reset the frame counter */
    FrameCount = 0;
    
    /* Check the data to be sent through IN pipe */
    Handle_USBAsynchXfer(pdev);
  }
  
  return OK;
}

/**
  * @brief  Handle_USBAsynchXfer
  *         Send data to USB
  * @param  pdev: instance
  * @retval None
  */
static void
Handle_USBAsynchXfer(void *pdev)
{
  if ( USB_Tx_State ) return;

  uint16_t USB_Tx_ptr = APP_Tx_ptr_out;
  uint16_t USB_Tx_length = (APP_Tx_ptr_in - USB_Tx_ptr) & APP_TX_DATA_SIZE_MASK;
  if ( USB_Tx_length==0 )
    return; // nothing to send

	usb_debug ( DM_WRITE1, "USB Tx asynch start: %d\n", USB_Tx_length );

    USB_Tx_State = 1;

    if (USB_Tx_length > CDC_DATA_IN_PACKET_SIZE) {
       USB_Tx_length = CDC_DATA_IN_PACKET_SIZE;
    }

    if ( USB_Tx_length > (APP_TX_DATA_SIZE - USB_Tx_ptr) ) {
       USB_Tx_length = (APP_TX_DATA_SIZE - USB_Tx_ptr);
    }

    APP_Tx_ptr_out = (USB_Tx_ptr+USB_Tx_length)&APP_TX_DATA_SIZE_MASK;

	usb_debug ( DM_WRITE1, "USB Tx asynch send: %d\n", USB_Tx_length );

    EP_Tx (pdev,
               CDC_IN_EP,
               (uint8_t*)&APP_Tx_Buffer[USB_Tx_ptr],
               USB_Tx_length);
}

/* THE END */
