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

#include "../hydra.h"
#include "hydra_usb.h"
#include "usb.h"

#include <stdarg.h>

#include <library/usbd_usr.h>
#include <vcp/usbd_cdc_core.h>
#include <vcp/usbd_desc.h>


USB_OTG_CORE_HANDLE  USB_OTG_dev;

/* A footnote on figure 26, page 272 of the TRM says
 * that the value 12 is used for the usb HS when used in
 * FS mode.
 */
#define GPIO_ALT_USB	10
#define GPIO_ALT_USB_FS	12

/* This is for the F411 and/or F429, F407 */
void
gpio_usb_init ( void )
{
        gpio_usb_pin_setup ( GPIOA, 11, GPIO_ALT_USB );   /* A11 - D- (DM) */
        gpio_usb_pin_setup ( GPIOA, 12, GPIO_ALT_USB );   /* A12 - D+ (DP) */

		/* This is the HS controller on the F429 discovery board
		 *  as well as on the F407 Olimex E407 board
		 * We just go ahead and configure these pins regardless
		 * (lazy for now anyway) even if we aren't going to use them.
		 */
        gpio_usb_pin_setup ( GPIOB, 14, GPIO_ALT_USB_FS );   /* B14 - D- (DM) */
        gpio_usb_pin_setup ( GPIOB, 15, GPIO_ALT_USB_FS );   /* B15 - D+ (DP) */
}

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

#define IRQ_USB_WAKEUP  42
#define IRQ_USB_FS      67

#define IRQ_USB_HS_EP1_OUT      74
#define IRQ_USB_HS_EP1_IN       75
#define IRQ_USB_HS_WAKEUP       76
#define IRQ_USB_HS      		77

#ifdef notdef
usb_hs_ep1_out      /* IRQ 74 */
usb_hs_ep1_in       /* IRQ 75 */
usb_hs_wakeup       /* IRQ 76 */
usb_hs_irq_handler  /* IRQ 77 */
#endif


		nvic_enable ( IRQ_USB_WAKEUP );
        nvic_enable ( IRQ_USB_FS );

		nvic_enable ( IRQ_USB_HS_EP1_OUT );
		nvic_enable ( IRQ_USB_HS_EP1_IN );
		nvic_enable ( IRQ_USB_HS_WAKEUP );
        nvic_enable ( IRQ_USB_HS );

        gpio_usb_init ();

/* Change this in usb_conf.h */
#ifdef USE_USB_OTG_HS
	  printf ( "Initialize HS usb core with IRQ %d\n", IRQ_USB_HS );
      USBD_Init(&USB_OTG_dev,
            USB_OTG_HS_CORE_ID,
            &USR_desc,
            &USBD_CDC_cb,
            &USR_cb);
#else
	  printf ( "Initialize FS usb core with IRQ %d\n", IRQ_USB_FS );
      USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            &USR_desc,
            &USBD_CDC_cb,
            &USR_cb);
#endif
}

/* Delay in microseconds
 */
void 
board_uDelay (const uint32_t usec)
{
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);

  do {
    if ( ++count > utime )
      return ;
  } while (1);
}


/* Delay in milliseconds
 */
void
board_mDelay (const uint32_t msec)
{
  board_uDelay ( msec * 1000 );
}

/* =========================================================================
 * USB debug facility
 */

#define PRINTF_BUF_SIZE 128

void asnprintf (char *abuf, unsigned int size, const char *fmt, va_list args);

// static int usb_debug_mask = DM_EVENT | DM_ENUM;
// static int usb_debug_mask = DM_READ1 | DM_ENUM | DM_EVENT;
// static int usb_debug_mask = DM_ENUM | DM_EVENT;
// static int usb_debug_mask = DM_READ1;
// static int usb_debug_mask = DM_ORIG;
// static int usb_debug_mask = DM_ORIG | DM_EVENT | DM_ENUM;
static int usb_debug_mask = 0;

void
usb_debug ( int select, char *fmt, ... )
{
        char buf[PRINTF_BUF_SIZE];
        va_list args;

		if ( ! (usb_debug_mask & select) )
			return;

        va_start ( args, fmt );
        asnprintf ( buf, PRINTF_BUF_SIZE, fmt, args );
        va_end ( args );

        puts ( buf );
}

void
usb_dump ( int select, char *msg, char *buf, int n )
{
		int i;

		if ( ! (usb_debug_mask & select) )
			return;

		puts ( msg );
		printf ( " (%d) ", n );
		for ( i=0; i<n; i++ )
			printf ( "%x", buf[i] );
		puts ( "\n" );
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

/* 5-16-2025 - data sending experiment */
void
usb_test_send ( void )
{
	char xbuf[1024];
	int i;
	int n = 800;

	for ( i=0; i<1024; i++ )
		xbuf[i] = 'A';

	for ( i=n-5; i<n; i++ )
		xbuf[i] = '-';

	(void) VCP_DataTx ( xbuf, n );
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

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* Interrupt handlers below here.
 * all called from locore_411.s
 */

/* The same handler for FS and HS -- for now.
 * Someday if we have both USB devices active
 * at the same time, they will both come here and
 * we will have to sort out which one interrupted.
 * Also the HS controller uses some other vectors
 * for its special EP-1 interrupts.
 */

void
usb_irq_handler ( void )
{
	// printf ( "FS interrupt\n" );
	USBD_OTG_ISR_Handler ( &USB_OTG_dev );
}

void
usb_wakeup_handler ( void )
{
	printf ( "USB FS wakeup interrupt\n" );
}

#ifdef notdef
usb_hs_ep1_out      /* IRQ 74 */
usb_hs_ep1_in       /* IRQ 75 */
usb_hs_wakeup       /* IRQ 76 */
#endif

void
usb_hs_irq_handler ( void )
{
	// printf ( "HS interrupt\n" );
	USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

void
usb_hs_ep1_out ( void )
{ 
#ifdef USE_USB_OTG_HS
	// printf ( "USB HS ep1 out interrupt\n" );
	(void) USBD_OTG_EP1OUT_ISR_Handler ( &USB_OTG_dev );
#endif
}
void
usb_hs_ep1_in ( void )
{
#ifdef USE_USB_OTG_HS
	// printf ( "USB HS ep1 in interrupt\n" );
	(void) USBD_OTG_EP1IN_ISR_Handler ( &USB_OTG_dev );
#endif
}

void
usb_hs_wakeup ( void )
{
	printf ( "USB HS wakeup interrupt\n" );
}

/* THE END */
