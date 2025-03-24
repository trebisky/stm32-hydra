/* desc.c
 * All the descriptors used during enumeration
 *  for this class.
 *
 * Tom Trebisky  3-24-2025
 *
 */
#include "hydra_usb.h"

#include "usbd_cdc_core.h"
#include "vcp/usbd_desc.h"
#include "library/usbd_req.h"

/* =========================================================================== */
/* =========================================================================== */
/* =========================================================================== */
/* The descriptor definitions come first */

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t usbd_cdc_CfgDesc[USB_CDC_CONFIG_DESC_SIZ]  __ALIGN_END =
{
  /*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
  USB_CDC_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
  0x00,
  0x02,   /* bNumInterfaces: 2 interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0xC0,   /* bmAttributes: self powered */
  0x32,   /* MaxPower 0 mA */
  
  /*---------------------------------------------------------------------------*/
  
  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */
  
  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,
  
  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */
  
  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */
  
  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */
  
  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SZE),
#ifdef USE_USB_OTG_HS
  0x10,                           /* bInterval: */
#else
  0xFF,                           /* bInterval: */
#endif /* USE_USB_OTG_HS */
  
  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
  0x00,                              /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  CDC_IN_EP,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
  HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
  0x00                               /* bInterval: ignore for Bulk transfer */
} ;

#ifdef USE_USB_OTG_HS
__ALIGN_BEGIN static uint8_t usbd_cdc_OtherCfgDesc[USB_CDC_CONFIG_DESC_SIZ]  __ALIGN_END =
{ 
  0x09,   /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,   
  USB_CDC_CONFIG_DESC_SIZ,
  0x00,
  0x02,   /* bNumInterfaces: 2 interfaces */
  0x01,   /* bConfigurationValue: */
  0x04,   /* iConfiguration: */
  0xC0,   /* bmAttributes: */
  0x32,   /* MaxPower 100 mA */  
  
  /*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */
  
  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,
  
  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */
  
  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */
  
  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */
  
  /*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
  CDC_CMD_EP,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(CDC_CMD_PACKET_SZE),     /* wMaxPacketSize: */
  HIBYTE(CDC_CMD_PACKET_SZE),
  0xFF,                           /* bInterval: */
  
  /*---------------------------------------------------------------------------*/
  
  /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */
  
  /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  CDC_OUT_EP,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  0x40,                              /* wMaxPacketSize: */
  0x00,
  0x00,                              /* bInterval: ignore for Bulk transfer */
  
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType: Endpoint */
  CDC_IN_EP,                        /* bEndpointAddress */
  0x02,                             /* bmAttributes: Bulk */
  0x40,                             /* wMaxPacketSize: */
  0x00,
  0x00                              /* bInterval */
};
#endif /* USE_USB_OTG_HS  */

/* =========================================================================== */
/* =========================================================================== */

int
class_get_descriptor ( uint8_t type, USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req,
		uint8_t **apbuf, uint16_t *alen )
{
  uint16_t len;
  uint8_t *pbuf;

  usb_debug ( DM_DESC, "Get Descriptor: %d (wLength = %d)\n", type, req->wLength );

  switch ( type ) {
	  case USB_DESC_TYPE_DEVICE:
		  pbuf = pdev->dev.usr_device->GetDeviceDescriptor(pdev->cfg.speed, &len);
		  if ((req->wLength == 64) ||( pdev->dev.device_status == USB_OTG_DEFAULT))
			  len = 8;
		  break;
    
	  case USB_DESC_TYPE_CONFIGURATION:
		  usb_debug ( DM_DESC, " ** getting CDC config descriptor **\n" );
		  // pbuf   = (uint8_t *)pdev->dev.class_cb->GetConfigDescriptor(pdev->cfg.speed, &len);
		  pbuf = usbd_cdc_CfgDesc;
		  len = sizeof (usbd_cdc_CfgDesc);
/* This only would be used if we are actually in full speed mode
 * with an external full speed Phy.
 */
#ifdef USB_OTG_HS_CORE
		  if ( (pdev->cfg.speed == USB_OTG_SPEED_FULL) && (pdev->cfg.phy_itface  == USB_OTG_ULPI_PHY) ) {
			  // pbuf   = (uint8_t *)pdev->dev.class_cb->GetOtherConfigDescriptor(pdev->cfg.speed, &len);
			  len = sizeof (usbd_cdc_OtherCfgDesc);
			  pbuf = usbd_cdc_OtherCfgDesc;
		  }
#endif  
		  pbuf[1] = USB_DESC_TYPE_CONFIGURATION;

		  pdev->dev.pConfig_descriptor = pbuf;    
		  break;
    
	  case USB_DESC_TYPE_STRING:
		  switch ((uint8_t)(req->wValue)) {
			  case USBD_IDX_LANGID_STR:
				  pbuf = pdev->dev.usr_device->GetLangIDStrDescriptor(pdev->cfg.speed, &len);        
				  break;
			  case USBD_IDX_MFC_STR:
				  pbuf = pdev->dev.usr_device->GetManufacturerStrDescriptor(pdev->cfg.speed, &len);
				  break;
			  case USBD_IDX_PRODUCT_STR:
				  pbuf = pdev->dev.usr_device->GetProductStrDescriptor(pdev->cfg.speed, &len);
				  break;
			  case USBD_IDX_SERIAL_STR:
				  pbuf = pdev->dev.usr_device->GetSerialStrDescriptor(pdev->cfg.speed, &len);
				  break;
			  case USBD_IDX_CONFIG_STR:
				  pbuf = pdev->dev.usr_device->GetConfigurationStrDescriptor(pdev->cfg.speed, &len);
				  break;
			  case USBD_IDX_INTERFACE_STR:
				  pbuf = pdev->dev.usr_device->GetInterfaceStrDescriptor(pdev->cfg.speed, &len);
				  break;
			  default:
#ifdef USB_SUPPORT_USER_STRING_DESC
				  pbuf = pdev->dev.class_cb->GetUsrStrDescriptor(pdev->cfg.speed, (req->wValue) , &len);
				  break;
#else      
				  return 0;
#endif
		  }
		  break;

	  case USB_DESC_TYPE_DEVICE_QUALIFIER:                   
#ifdef USB_OTG_HS_CORE
		  if ( pdev->cfg.speed == USB_OTG_SPEED_HIGH )   {
			  pbuf   = (uint8_t *)pdev->dev.class_cb->GetConfigDescriptor(pdev->cfg.speed, &len);
            
			  USBD_DeviceQualifierDesc[4]= pbuf[14];
			  USBD_DeviceQualifierDesc[5]= pbuf[15];
			  USBD_DeviceQualifierDesc[6]= pbuf[16];
      
			  pbuf = USBD_DeviceQualifierDesc;
			  len  = USB_LEN_DEV_QUALIFIER_DESC;
			  break;
		  } else {
		  	  return 0;
		  }
#else
	  return 0;
#endif    

	  case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
#ifdef USB_OTG_HS_CORE   
		  if ( pdev->cfg.speed == USB_OTG_SPEED_HIGH )   {
			  pbuf   = (uint8_t *)pdev->dev.class_cb->GetOtherConfigDescriptor(pdev->cfg.speed, &len);
			  pbuf[1] = USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION;
			  break; 
		  } else {
			  return 0;
		  }
#else
		  return 0;
#endif     

	  default: 
	  	  return 0;
  }		/* end of big switch */

  *apbuf = pbuf;
  *alen = len;
  return 1;
}

/* =========================================================================== */
/* =========================================================================== */

/* XXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXX */

#ifdef not_a_chance

/*********************************************
   CDC Device library callbacks
 *********************************************/
static uint8_t  usbd_cdc_Init        (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_DeInit      (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_Setup       (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_cdc_EP0_RxReady  (void *pdev);
static uint8_t  usbd_cdc_DataIn      (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_DataOut     (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_SOF         (void *pdev);

/*********************************************
   CDC specific management functions
 *********************************************/
static void Handle_USBAsynchXfer  (void *pdev);
static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length);

#ifdef USE_USB_OTG_HS  
static uint8_t  *USBD_cdc_GetOtherCfgDesc (uint8_t speed, uint16_t *length);
#endif

extern CDC_IF_Prop_TypeDef  APP_FOPS;

extern uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC];

__ALIGN_BEGIN uint8_t usbd_cdc_CfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END ;

__ALIGN_BEGIN uint8_t usbd_cdc_OtherCfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END ;

__ALIGN_BEGIN static __IO uint32_t  usbd_cdc_AltSet  __ALIGN_END = 0;

__ALIGN_BEGIN uint8_t __CCMRAM__ USB_Rx_Buffer[CDC_DATA_MAX_PACKET_SIZE] __ALIGN_END ;

/* tjt - XXX - it is odd that only the Tx buffer is not specified as
 * being in CCMRAM - and also note that depending on the linker file,
 * it could end up in CCMRAM anyway, this is just a hint to say
 * "put it ih CCMRAM if possible" -- however CCMRAM is supposed to
 * not work with DMA, so there could be trouble waiting here.
 */
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
__ALIGN_BEGIN uint8_t APP_Tx_Buffer   [APP_TX_DATA_SIZE] __ALIGN_END ;
#else 
__ALIGN_BEGIN uint8_t __CCMRAM__ APP_Tx_Buffer[APP_TX_DATA_SIZE] __ALIGN_END ;
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
 
__ALIGN_BEGIN uint8_t CmdBuff[CDC_CMD_PACKET_SZE] __ALIGN_END ;

volatile uint16_t APP_Tx_ptr_in  = 0;
volatile uint16_t APP_Tx_ptr_out = 0;

uint8_t  USB_Tx_State = 0;

static uint32_t cdcCmd = 0xFF;
static uint32_t cdcLen = 0;

/* CDC interface class callbacks structure */
USBD_Class_cb_TypeDef  USBD_CDC_cb = 
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
  USBD_cdc_GetCfgDesc,
#ifdef USE_USB_OTG_HS   
  USBD_cdc_GetOtherCfgDesc, /* use same cobfig as per FS */
#endif /* USE_USB_OTG_HS  */
};

/**
  * @brief  usbd_cdc_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_cdc_Init (void  *pdev, 
                               uint8_t cfgidx)
{
  uint8_t *pbuf;

  /* Open EP IN */
  DCD_EP_Open(pdev,
              CDC_IN_EP,
              CDC_DATA_IN_PACKET_SIZE,
              USB_OTG_EP_BULK);
  
  /* Open EP OUT */
  DCD_EP_Open(pdev,
              CDC_OUT_EP,
              CDC_DATA_OUT_PACKET_SIZE,
              USB_OTG_EP_BULK);
  
  /* Open Command IN EP */
  DCD_EP_Open(pdev,
              CDC_CMD_EP,
              CDC_CMD_PACKET_SZE,
              USB_OTG_EP_INT);
  
  pbuf = (uint8_t *)USBD_DeviceDesc;
  pbuf[4] = DEVICE_CLASS_CDC;
  pbuf[5] = DEVICE_SUBCLASS_CDC;
  
  /* Initialize the Interface physical components */
  APP_FOPS.pIf_Init(pdev);

  /* Prepare Out endpoint to receive next packet */
  DCD_EP_PrepareRx(pdev,
                   CDC_OUT_EP,
                   (uint8_t*)(USB_Rx_Buffer),
                   CDC_DATA_OUT_PACKET_SIZE);
  
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_cdc_DeInit (void  *pdev, 
                                 uint8_t cfgidx)
{
  /* Open EP IN */
  DCD_EP_Close(pdev,
              CDC_IN_EP);
  
  /* Open EP OUT */
  DCD_EP_Close(pdev,
              CDC_OUT_EP);
  
  /* Open Command IN EP */
  DCD_EP_Close(pdev,
              CDC_CMD_EP);

  /* Restore default state of the Interface physical components */
  APP_FOPS.pIf_DeInit();
  
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_cdc_Setup (void  *pdev, 
                                USB_SETUP_REQ *req)
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
          APP_FOPS.pIf_Ctrl(req->bRequest, CmdBuff, req->wLength);
          
          /* Send the data to the host */
          USBD_CtlSendData (pdev, 
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
          USBD_CtlPrepareRx (pdev,
                             CmdBuff,
                             req->wLength);          
        }
      }
      else /* No Data request */
      {
        /* Transfer the command to the interface layer */
        APP_FOPS.pIf_Ctrl(req->bRequest, (uint8_t*)&req->wValue, sizeof(req->wValue));
      }
      
      return USBD_OK;
      
    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( (req->wValue >> 8) == CDC_DESCRIPTOR_TYPE)
      {
        uint8_t  *pbuf;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
		/* XXX - trouble here, where is this? */
        pbuf = usbd_cdc_Desc;   
#else
        pbuf = usbd_cdc_CfgDesc + 9 + (9 * USBD_ITF_MAX_NUM);
#endif 
        uint16_t len = MIN(USB_CDC_DESC_SIZ , req->wLength);
      
        USBD_CtlSendData (pdev, 
                          pbuf,
                          len);
      }
      break;
      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&usbd_cdc_AltSet,
                        1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
      {
        usbd_cdc_AltSet = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  usbd_cdc_EP0_RxReady (void  *pdev)
{ 
  if (cdcCmd != NO_CMD)
  {
    /* Process the data */
    APP_FOPS.pIf_Ctrl(cdcCmd, CmdBuff, cdcLen);
    
    /* Reset the command variable to default value */
    cdcCmd = NO_CMD;
  }
  
  return USBD_OK;
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

static uint8_t
usbd_cdc_DataIn (void *pdev, uint8_t epnum)
{
	if (USB_Tx_State == 0) return USBD_OK;

    uint16_t USB_Tx_ptr = APP_Tx_ptr_out;
    uint16_t USB_Tx_length = (APP_Tx_ptr_in - USB_Tx_ptr) & APP_TX_DATA_SIZE_MASK;

    usb_debug ( DM_ORIG, "usbd_cdc_DataIn called with %d bytes waiting to send on endpoint %d\n", USB_Tx_length, epnum );

    if (USB_Tx_length == 0)
    {
        //USB_Tx_State = 0;
        if (((USB_OTG_CORE_HANDLE*)pdev)->dev.in_ep[epnum].xfer_len != CDC_DATA_IN_PACKET_SIZE)
        {
            USB_Tx_State = 0;
            return USBD_OK;
        }
        /* Transmit zero sized packet in case the last one has maximum allowed size. Otherwise
         * the recipient may expect more data coming soon and not return buffered data to app.
         * See section 5.8.3 Bulk Transfer Packet Size Constraints
         * of the USB Specification document.
        */
    }

	usb_debug ( DM_WRITE1, "USB Tx datain start: %d\n", USB_Tx_length );

    if (USB_Tx_length > CDC_DATA_IN_PACKET_SIZE)
    {
       USB_Tx_length = CDC_DATA_IN_PACKET_SIZE;
    }
    if ( USB_Tx_length > (APP_TX_DATA_SIZE - USB_Tx_ptr) )
    {
       USB_Tx_length = (APP_TX_DATA_SIZE - USB_Tx_ptr);
    }

    APP_Tx_ptr_out = (USB_Tx_ptr + USB_Tx_length) & APP_TX_DATA_SIZE_MASK;

	usb_debug ( DM_WRITE1, "USB Tx datain send: %d\n", USB_Tx_length );

    /* Prepare the available data buffer to be sent on IN endpoint */
    DCD_EP_Tx (pdev,
                 CDC_IN_EP,
                 (uint8_t*)&APP_Tx_Buffer[USB_Tx_ptr],
                 USB_Tx_length);

  return USBD_OK;
}

void usbd_cdc_PrepareRx (void *pdev)
{
    DCD_EP_PrepareRx(pdev, CDC_OUT_EP, USB_Rx_Buffer, CDC_DATA_OUT_PACKET_SIZE);
}

/**
  * @brief  usbd_cdc_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t usbd_cdc_DataOut(void *pdev, uint8_t epnum)
{      
  /* Get the received data buffer and update the counter */
  uint16_t USB_Rx_Cnt = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;
  
  /* USB data will be immediately processed, this allow next USB traffic being 
     NAKed till the end of the application Xfer */
  if ( APP_FOPS.pIf_DataRx(USB_Rx_Buffer, USB_Rx_Cnt)==USBD_OK )
  {
    /* Prepare Out endpoint to receive next packet */
    DCD_EP_PrepareRx(pdev, CDC_OUT_EP, USB_Rx_Buffer, CDC_DATA_OUT_PACKET_SIZE);
  }
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_SOF
  *         Start Of Frame event management
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_cdc_SOF(void *pdev)
{      
  static uint32_t FrameCount = 0;
  
  if (FrameCount++ == CDC_IN_FRAME_INTERVAL)
  {
    /* Reset the frame counter */
    FrameCount = 0;
    
    /* Check the data to be sent through IN pipe */
    Handle_USBAsynchXfer(pdev);
  }
  
  return USBD_OK;
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
  {
    return; // nothing to send
  }

	usb_debug ( DM_WRITE1, "USB Tx asynch start: %d\n", USB_Tx_length );

    USB_Tx_State = 1;

    if (USB_Tx_length > CDC_DATA_IN_PACKET_SIZE)
    {
       USB_Tx_length = CDC_DATA_IN_PACKET_SIZE;
    }
    if ( USB_Tx_length > (APP_TX_DATA_SIZE - USB_Tx_ptr) )
    {
       USB_Tx_length = (APP_TX_DATA_SIZE - USB_Tx_ptr);
    }

    APP_Tx_ptr_out = (USB_Tx_ptr+USB_Tx_length)&APP_TX_DATA_SIZE_MASK;

	usb_debug ( DM_WRITE1, "USB Tx asynch send: %d\n", USB_Tx_length );

    DCD_EP_Tx (pdev,
               CDC_IN_EP,
               (uint8_t*)&APP_Tx_Buffer[USB_Tx_ptr],
               USB_Tx_length);
}

/**
  * @brief  USBD_cdc_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_cdc_CfgDesc);
  return usbd_cdc_CfgDesc;
}

/**
  * @brief  USBD_cdc_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
#ifdef USE_USB_OTG_HS 
static uint8_t  *USBD_cdc_GetOtherCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_cdc_OtherCfgDesc);
  return usbd_cdc_OtherCfgDesc;
}
#endif

#endif /* not_a_chance */

/* THE END */
