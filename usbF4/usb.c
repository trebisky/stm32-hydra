#ifdef notdef
#include <STM32_USB_Device_Library/Class/cdc/inc/usbd_cdc_core.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_usr.h>
#include <VCP/usbd_desc.h>
#include "usb.h"
#include <libmaple/gpio.h>
#include <libmaple/rccF4.h>
#include <VCP/usbd_cdc_vcp.h>
#include <boards.h>
#include <STM32_USB_OTG_Driver/inc/usb_dcd_int.h>
#endif

#include <library/usbd_cdc_core.h>
#include <library/usbd_usr.h>
#include <vcp/usbd_desc.h>

#include "usb.h"

USB_OTG_CORE_HANDLE  USB_OTG_dev;

void
usb_init (void)
{

#ifdef notdef
      gpio_set_mode(BOARD_USB_DP_PIN, GPIO_OUTPUT_OD); // ala42

      gpio_clear_pin(BOARD_USB_DP_PIN); // ala42
      delay_us(50000);

      /* initialize the usb application */
      gpio_set_pin(BOARD_USB_DP_PIN); // ala42 // presents us to the host
#endif

#ifdef HYDRA

#define IRQ_USB_WAKEUP  42
#define IRQ_USB_FS      67

	nvic_enable ( IRQ_USB_WAKEUP );
        nvic_enable ( IRQ_USB_FS );

        gpio_usb_init ();
#endif


      USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            &USR_desc,
            &USBD_CDC_cb,
            &USR_cb);
}

// This should work, but it doesn't
//#define strlen(x)          __builtin_strlen ((x))

#ifdef notdef
static int
mystrlen ( char *s )
{
	int rv = 0;

	while ( *s++ )
	    ++rv;
	return rv;
}
#endif

void
usb_puts ( char *buf )
{
	char ubuf[128];
	char *p;
	int len;

	p = ubuf;
	len = 0;

	while ( *buf ) {
	    if ( *buf == '\n' ) {
		*p++ = '\r';
		len++;
	    }
	    *p++ = *buf++;
	    len++;
	}

	// (void) VCP_DataTx ( buf, strlen(buf) );
	// (void) VCP_DataTx ( buf, __builtin_strlen(buf) );
	(void) VCP_DataTx ( ubuf, len );
}

/* This works fine, as expected */
void
usb_write ( char *buf, int len )
{
	(void) VCP_DataTx ( buf, len );
}

/* This never blocks and returns 0 at a ferrocious rate. */
int
usb_read ( char *buf, int len )
{
	return VCPGetBytes ( buf, len );
}

#ifdef notyet
extern uint16_t VCP_DataTx (const uint8_t* Buf, uint32_t Len);
extern uint8_t  VCPGetByte(void);
extern uint32_t VCPGetBytes(uint8_t * rxBuf, uint32_t len);

uint32_t usbSendBytes(const uint8_t* sendBuf, uint32_t len)
{
	return VCP_DataTx(sendBuf, len);
}

uint32_t usbReceiveBytes(uint8_t* recvBuf, uint32_t len)
{
	return VCPGetBytes(recvBuf, len);
}

RESULT usbPowerOff(void)
{
	USBD_DeInitFull(&USB_OTG_dev);
	return USB_SUCCESS;
}
#endif

void
usb_irq_handler ( void )
{
	USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

void
usb_wakeup_handler ( void )
{
}

#ifndef HYDRA
void __irq_usb_fs(void)
{
	USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

void x__irq_usbwakeup(void)
{
}
#endif

/* THE END */
