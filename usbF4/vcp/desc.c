/* desc.c
 * All the descriptors used during enumeration
 *  for this class.
 *
 * Tom Trebisky  3-24-2025
 *
 */
#include "hydra_usb.h"

#include <library/usbd_def.h>
#include "usbd_conf.h"


#include "usbd_cdc_core.h"
// #include "vcp/usbd_desc.h"
#include "library/usbd_req.h"

/* From the old usbd_desc.c */
// #include <library/usbd_core.h>
// #include "usbd_desc.h"
// #include <library/usbd_req.h>

/* =========================================================================== */

#define USBD_VID                        0x0483
#define USBD_PID                        0x5740

#define USBD_LANGID_STRING              0x409

// #define USBD_MANUFACTURER_STRING        (uint8_t*)"STMicroelectronics"
#define USBD_MANUFACTURER_STRING        (uint8_t*)"ACME bar and grill"

#define USBD_PRODUCT_HS_STRING          (uint8_t*)"STM32 Virtual ComPort in HS Mode"
#define USBD_SERIALNUMBER_HS_STRING     (uint8_t*)"00000000050B"

#define USBD_PRODUCT_FS_STRING          (uint8_t*)"STM32 Virtual ComPort in FS Mode"
#define USBD_SERIALNUMBER_FS_STRING     (uint8_t*)"00000000050C"

#define USBD_CONFIGURATION_HS_STRING    (uint8_t*)"VCP Config"
#define USBD_INTERFACE_HS_STRING        (uint8_t*)"VCP Interface"

#define USBD_CONFIGURATION_FS_STRING    (uint8_t*)"VCP Config"
#define USBD_INTERFACE_FS_STRING        (uint8_t*)"VCP Interface"

#define USB_SIZ_DEVICE_DESC                     18
#define USB_SIZ_STRING_LANGID                   4

/* A place to build unicode from string descriptors */
__ALIGN_BEGIN uint8_t unibuf[USB_MAX_STR_DESC_SIZ] __ALIGN_END ;

/* =========================================================================== */
/* =========================================================================== */

/* The descriptor definitions come first */

#ifdef notdef
  /* This used to be "hacked in" by code in usbd_cdc_core.c
   * Now I just "make it so" in the data below.
   */
  pbuf = (uint8_t *) USBD_DeviceDesc;
  pbuf[4] = DEVICE_CLASS_CDC;
  pbuf[5] = DEVICE_SUBCLASS_CDC;
#endif

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
#ifdef notdef
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
#endif
    DEVICE_CLASS_CDC,           /*bDeviceClass*/
    DEVICE_SUBCLASS_CDC,        /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,       /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_CFG_MAX_NUM            /*bNumConfigurations*/
  } ; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
     USB_SIZ_STRING_LANGID,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};

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

/* This only would be used if we are actually in full speed mode
 * with an external full speed Phy.
 */
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

#ifdef notdef
static void
USBD_GetString(uint8_t *desc, uint8_t *unicode, uint16_t *len)
{
  uint8_t idx = 0;

  if (desc != NULL) {
    *len =  USBD_GetLen(desc) * 2 + 2;
    unicode[idx++] = *len;
    unicode[idx++] =  USB_DESC_TYPE_STRING;

    while (*desc != NULL) {
      unicode[idx++] = *desc++;
      unicode[idx++] =  0x00;
    }
  }
}
#endif

/* Same as strlen ? */
/* XXX maybe just use sizeof(str)-1 */
static uint8_t
USBD_GetLen(uint8_t *buf)
{
    uint8_t  len = 0;

    while (*buf != NULL) {
        len++;
        buf++;
    }

    return len;
}


/* Convert string to unicode (yields double its size + 2).
 */
static uint16_t
GetString(uint8_t *desc, uint8_t *unicode )
{
    uint8_t idx = 0;
    uint16_t len = 0;

    if ( desc == NULL )
		return 0;

    len =  USBD_GetLen(desc) * 2 + 2;
    unicode[idx++] = len;
    unicode[idx++] =  USB_DESC_TYPE_STRING;

    while (*desc != NULL) {
        unicode[idx++] = *desc++;
        unicode[idx++] =  0x00;
    }

  return len;
}
int
class_get_descriptor ( uint8_t type, USB_OTG_CORE_HANDLE *pdev, USB_SETUP_REQ *req,
		uint8_t **apbuf, uint16_t *alen )
{
  uint16_t len;
  uint8_t *pbuf;
  uint8_t speed = pdev->cfg.speed;

  usb_debug ( DM_DESC, "Get Descriptor: %d (wLength = %d)\n", type, req->wLength );

  switch ( type ) {
	  case USB_DESC_TYPE_DEVICE:
		  // pbuf = pdev->dev.usr_device->GetDeviceDescriptor(pdev->cfg.speed, &len);
		  pbuf = USBD_DeviceDesc;
		  len = sizeof(USBD_DeviceDesc);
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
				  // pbuf = pdev->dev.usr_device->GetLangIDStrDescriptor(pdev->cfg.speed, &len);        
				  pbuf = USBD_LangIDDesc;
				  len =  sizeof(USBD_LangIDDesc);  
				  break;
			  case USBD_IDX_MFC_STR:
				  // pbuf = pdev->dev.usr_device->GetManufacturerStrDescriptor(pdev->cfg.speed, &len);
				  len = GetString ( USBD_MANUFACTURER_STRING, unibuf );
				  pbuf = unibuf;
				  break;
			  case USBD_IDX_PRODUCT_STR:
				  // pbuf = pdev->dev.usr_device->GetProductStrDescriptor(pdev->cfg.speed, &len);
				  if ( speed == USB_OTG_SPEED_HIGH )
					  len = GetString ( USBD_PRODUCT_HS_STRING, unibuf );
				  else
					  len = GetString ( USBD_PRODUCT_FS_STRING, unibuf );
				  pbuf = unibuf;
				  break;
			  case USBD_IDX_SERIAL_STR:
				  // pbuf = pdev->dev.usr_device->GetSerialStrDescriptor(pdev->cfg.speed, &len);
				  if ( speed == USB_OTG_SPEED_HIGH )
					  len = GetString ( USBD_SERIALNUMBER_HS_STRING, unibuf );
				  else
					  len = GetString ( USBD_SERIALNUMBER_FS_STRING, unibuf );
				  pbuf = unibuf;
				  break;
			  case USBD_IDX_CONFIG_STR:
				  // pbuf = pdev->dev.usr_device->GetConfigurationStrDescriptor(pdev->cfg.speed, &len);
				  if ( speed == USB_OTG_SPEED_HIGH )
					  len = GetString ( USBD_CONFIGURATION_HS_STRING, unibuf );
				  else
					  len = GetString ( USBD_CONFIGURATION_FS_STRING, unibuf );
				  pbuf = unibuf;
				  break;
			  case USBD_IDX_INTERFACE_STR:
				  // pbuf = pdev->dev.usr_device->GetInterfaceStrDescriptor(pdev->cfg.speed, &len);
				  if ( speed == USB_OTG_SPEED_HIGH )
					  len = GetString ( USBD_INTERFACE_HS_STRING, unibuf );
				  else
					  len = GetString ( USBD_INTERFACE_FS_STRING, unibuf );
				  pbuf = unibuf;
				  break;
			  default:
#ifdef USB_SUPPORT_USER_STRING_DESC
				  // never set up.
				  // pbuf = pdev->dev.class_cb->GetUsrStrDescriptor(pdev->cfg.speed, (req->wValue) , &len);
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

#ifdef OLD_STUFF
USBD_DEVICE USR_desc =
{
  USBD_USR_DeviceDescriptor,
  USBD_USR_LangIDStrDescriptor, 
  USBD_USR_ManufacturerStrDescriptor,
  USBD_USR_ProductStrDescriptor,
  USBD_USR_SerialStrDescriptor,
  USBD_USR_ConfigStrDescriptor,
  USBD_USR_InterfaceStrDescriptor,
};


/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_DeviceDesc);
  return USBD_DeviceDesc;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_LangIDDesc);  
  return USBD_LangIDDesc;
}


/**
* @brief  USBD_USR_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
 
  
  if(speed == 0)
  {   
    USBD_GetString (USBD_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {    
    USBD_GetString (USBD_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {  
    USBD_GetString (USBD_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
  }
  return USBD_StrDesc;  
}


/**
* @brief  USBD_USR_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString (USBD_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;  
}
#endif

/* THE END */
