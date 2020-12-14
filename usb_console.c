/* A usb console (serial driver)
 *
 * usb_console.c
 *
 * The idea here is that this is a chip independent
 * higher level driver for a "virtual console"
 *
 * It handles both calls from above (user code)
 * and upcalls from the USB driver below.
 *
 *  Tom Trebisky  12-13-2020
 */

#include "hydra.h"

void
usb_console_init ( void )
{
}

/* ================================================================== */

/* Below here is code to support upcalls from the low level USB driver*/

/* These values indicate this is the Virtual Console driver from ST */
#define USBD_VID                        0x0483
#define USBD_PID                        0x5740

#define USBD_MANUFACTURER_STRING        (uint8_t*)"STMicroelectronics"

#define USBD_PRODUCT_FS_STRING          (uint8_t*)"STM32 Virtual ComPort in FS Mode"
#define USBD_SERIALNUMBER_FS_STRING     (uint8_t*)"00000000050C"

#define USBD_CONFIGURATION_FS_STRING    (uint8_t*)"VCP Config"
#define USBD_INTERFACE_FS_STRING        (uint8_t*)"VCP Interface"

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define  USB_REQ_RECIPIENT_DEVICE                       0x00
#define  USB_REQ_RECIPIENT_INTERFACE                    0x01
#define  USB_REQ_RECIPIENT_ENDPOINT                     0x02
#define  USB_REQ_RECIPIENT_MASK                         0x03

#define  USB_REQ_GET_STATUS                             0x00
#define  USB_REQ_CLEAR_FEATURE                          0x01
#define  USB_REQ_SET_FEATURE                            0x03
#define  USB_REQ_SET_ADDRESS                            0x05
#define  USB_REQ_GET_DESCRIPTOR                         0x06
#define  USB_REQ_SET_DESCRIPTOR                         0x07
#define  USB_REQ_GET_CONFIGURATION                      0x08
#define  USB_REQ_SET_CONFIGURATION                      0x09
#define  USB_REQ_GET_INTERFACE                          0x0A
#define  USB_REQ_SET_INTERFACE                          0x0B
#define  USB_REQ_SYNCH_FRAME                            0x0C

#define  USB_DESC_TYPE_DEVICE                              1
#define  USB_DESC_TYPE_CONFIGURATION                       2
#define  USB_DESC_TYPE_STRING                              3
#define  USB_DESC_TYPE_INTERFACE                           4
#define  USB_DESC_TYPE_ENDPOINT                            5
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                    6
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION           7
#define  USB_DESC_TYPE_BOS                                 0x0F

#define  USB_REQ_TYPE_STANDARD                          0x00
#define  USB_REQ_TYPE_CLASS                             0x20
#define  USB_REQ_TYPE_VENDOR                            0x40
#define  USB_REQ_TYPE_MASK                              0x60

#define  USBD_IDX_LANGID_STR                            0x00
#define  USBD_IDX_MFC_STR                               0x01
#define  USBD_IDX_PRODUCT_STR                           0x02
#define  USBD_IDX_SERIAL_STR                            0x03
#define  USBD_IDX_CONFIG_STR                            0x04
#define  USBD_IDX_INTERFACE_STR                         0x05

#define LOBYTE(x)  ((unsigned char)((x) & 0x00FF))
#define HIBYTE(x)  ((unsigned char)(((x) & 0xFF00) >>8))

/* XXX needs to match things in driver*/
#define USB_OTG_MAX_EP0_SIZE                 64

/* Better be 18 bytes.
 * Cannot use strlen on this (or on unicode)
 * due to embedded zero bytes.
 */
#define LEN_DEVICE_DESC	18

static char USBD_DeviceDesc[] =
{
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/

    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idVendor*/
    HIBYTE(USBD_PID),           /*idVendor*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    1			        /*bNumConfigurations*/
} ; /* USB_DeviceDescriptor */

#define USBD_LANGID_STRING              0x409

/* USB Standard Device Descriptor */
static char USBD_LangIDDesc[]  =
{
     4,         
     USB_DESC_TYPE_STRING,       
     LOBYTE(USBD_LANGID_STRING),
     HIBYTE(USBD_LANGID_STRING), 
};

/* ------------------------------------------------------------- */

#ifdef notdef
static int
usb_strlen ( char *buf )
{
    int  rv = 0;

    while ( *buf++ )
        rv++;

    return rv;
}
#endif

/* A setup packet looks like this */
struct usb_setup {
	unsigned char bmRequest;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
};

static void
usb_string_reply ( char *str )
{
    char ustr[128];
    int idx = 0;

    if ( ! str )
      return;

    // ustr[0] = 0;
    ustr[1] =  USB_DESC_TYPE_STRING;
    idx = 2;

    while ( *str ) {
      ustr[idx++] = *str++;
      ustr[idx++] =  0x00;
    }

    ustr[0] = idx;
    usb_write_data ( ustr, idx );
}


static int
usb_console_desc ( struct usb_setup *sp )
{
	char *reply;
	int len;

	switch ( sp->wValue >> 8 ) {
	    case USB_DESC_TYPE_DEVICE:
		reply = USBD_DeviceDesc;
		len = LEN_DEVICE_DESC;
		break;
	    default:
		return 0;
	}

	printf ( "Bingo (%d)!!\n", len );
	usb_write_data ( reply, len );
	return 1;
}

/* This gets called with every setup packet
 * and gets the first shot at them.
 * If it recognizes and handles them, it returns 1, else 0.
 * These always have a length of 8 bytes.
 */
int
usb_console_setup ( struct usb_setup *sp )
{
	if ( sp->bmRequest & 0x1f != USB_REQ_RECIPIENT_DEVICE )
	    return 0;

	/* other cases:
	  USB_REQ_RECIPIENT_INTERFACE
	  USB_REQ_RECIPIENT_ENDPOINT
	*/
	switch (sp->bRequest) {
	    case USB_REQ_GET_DESCRIPTOR:
		return usb_console_desc ( sp );
	    case USB_REQ_SET_ADDRESS:
	    case USB_REQ_SET_CONFIGURATION:
	    case USB_REQ_GET_CONFIGURATION:
	    case USB_REQ_GET_STATUS:
	    case USB_REQ_SET_FEATURE:
	    case USB_REQ_CLEAR_FEATURE:
	    default:
		// USBD_CtlError(pdev , req);
		break;
	}

	printf ( " ---- I don't know this one\n" );
	return 0;
}

/* OLD accessors */
#ifdef OLD

static int
USBD_GetLen(char *buf)
{
    int  len = 0;

    while (*buf++ != NULL)
        len++;

    return len;
}

/**
  * @brief  USBD_GetString
  *         Convert Ascii string into unicode one
  * @param  desc : descriptor buffer
  * @param  unicode : Formatted string buffer (unicode)
  * @param  len : descriptor length
  * @retval None
  */
void
USBD_GetString(uint8_t *desc, uint8_t *unicode, uint16_t *len)
{
    int idx = 0;

    if ( desc == NULL)
      return;

    *len =  USBD_GetLen(desc) * 2 + 2;
    unicode[idx++] = *len;
    unicode[idx++] =  USB_DESC_TYPE_STRING;

    while (*desc != NULL) {
      unicode[idx++] = *desc++;
      unicode[idx++] =  0x00;
    }
}


uint8_t *  USBD_USR_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_DeviceDesc);
  return USBD_DeviceDesc;
}

uint8_t *  USBD_USR_LangIDStrDescriptor( uint8_t speed , uint16_t *length) {}

uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  }
  return USBD_StrDesc;
}

uint8_t *  USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

uint8_t *  USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
  return USBD_StrDesc;
}

uint8_t *  USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
  return USBD_StrDesc;  
}

uint8_t *  USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;  
}

#endif

/* THE END */



