/* --------------------
 * public.c
 * Tom Trebisky  3-23-2025
 */

#include "types.h"
#include "usb_conf.h"

#include <stdarg.h>

#include "usb_core.h"
#include "protos.h"

// #include "usbd_core.h"

typedef void (*bfptr) ( char *, int );

// static void gpio_usb_init ( void );

int fusb_init ( void );
void fusb_puts ( int, char * );
void fusb_write ( int, char *, int );
int fusb_read ( int, char *, int );

/* ============================================================================== */
/* ============================================================================== */
/* First we have routines exposed to the outside world (upstream) */

/* The idea here is that the only functions exposed to the outside
 * world are defined in this file.
 *
 * These typically begin with usb_ (like usb_init, usb_puts, ... )
 */

static int usb_fd;

void
usb_init ( void )
{
		usb_fd = fusb_init ();
}

void
usb_puts ( char *msg )
{
		fusb_puts ( usb_fd, msg );
}

void
usb_write ( int fd, char *buf, int len )
{
		fusb_write ( usb_fd, buf, len );
}

int
usb_read ( char *buf, int len )
{
		return fusb_read ( usb_fd, buf, len );
}

void
usb_hookup ( bfptr fn )
{
		class_usb_hookup ( fn );
}

/* ============================================================================== */
/* ============================================================================== */
/* Next we have things called from the above that are "glue" to the
* private routines in the USB code.
*/

HANDLE  dev;

/* For now, this can only initialize one interface.
 * someday we want to be able to call this twice,
 * once for each interface.
 */
int
fusb_init (void)
{

#ifdef notdef
      gpio_set_mode(BOARD_USB_DP_PIN, GPIO_OUTPUT_OD); // ala42

      gpio_clear_pin(BOARD_USB_DP_PIN); // ala42
      // delay_us(50000);
      delay_ms(50);

      /* initialize the usb application */
      gpio_set_pin(BOARD_USB_DP_PIN); // ala42 // presents us to the host
#endif

#define IRQ_USB_WAKEUP  42
#define IRQ_USB_FS      67

#define IRQ_USB_HS_EP1_OUT      74
#define IRQ_USB_HS_EP1_IN       75
#define IRQ_USB_HS_WAKEUP       76
#define IRQ_USB_HS      		77

		nvic_enable ( IRQ_USB_WAKEUP );
        nvic_enable ( IRQ_USB_FS );

		nvic_enable ( IRQ_USB_HS_EP1_OUT );
		nvic_enable ( IRQ_USB_HS_EP1_IN );
		nvic_enable ( IRQ_USB_HS_WAKEUP );
        nvic_enable ( IRQ_USB_HS );

		/* set up GPIO alt function for USB */
        gpio_usb_init ();

		/* Doesn't currently do anything */
		class_init ();

/* Change this choice in usb_conf.h */

#ifdef USE_HS
	  printf ( "Initialize HS usb core with IRQ %d\n", IRQ_USB_HS );
      // Init(&dev, HS_CORE_ID, &USR_cb );
      BOGUS_Init(&dev, HS_CORE_ID );
#else
	  printf ( "Initialize FS usb core with IRQ %d\n", IRQ_USB_FS );
      // Init(&dev, FS_CORE_ID, &USR_cb );
      BOGUS_Init(&dev, FS_CORE_ID );
#endif

		/* XXX someday will be 0 or 1 */
		return 0;
}

#ifdef notdef
void
usbPowerOff ( void )
{
	DeInitFull(&dev);
}
#endif

void
fusb_puts ( int fd, char *buf )
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
	// (void) VCP_DataTx ( ubuf, len );
	fusb_write ( fd, ubuf, len );
}

/* This works fine, as expected */
void
fusb_write ( int fd, char *buf, int len )
{
	// (void) VCP_DataTx ( buf, len );
	class_usb_write ( buf, len );
}

/* This never blocks and returns 0 at a ferrocious rate. */
int
fusb_read ( int fd, char *buf, int len )
{
	// return VCPGetBytes ( buf, len );
	return class_usb_read ( buf, len );
}

/* ============================================================================== */
/* ============================================================================== */
/* Lastly we have routines that are internal "utility" routines used
 * by the USB code, but never seen by the outside world.
 * This includes our interrupt glue routines
 */

/* =========================================================================
 * USB debug facility (used only from within the USB code)
 */

#define PRINTF_BUF_SIZE 128

void asnprintf (char *abuf, unsigned int size, const char *fmt, va_list args);

// static int usb_debug_mask = DM_EVENT | DM_ENUM;
// static int usb_debug_mask = DM_READ1 | DM_ENUM | DM_EVENT;
// static int usb_debug_mask = DM_ENUM | DM_EVENT;
// static int usb_debug_mask = DM_READ1;
// static int usb_debug_mask = DM_ORIG;
// static int usb_debug_mask = DM_ORIG | DM_EVENT | DM_ENUM;
static int usb_debug_mask = DM_DESC | DM_ENUM | DM_EVENT;;
// static int usb_debug_mask = DM_ALL;
// static int usb_debug_mask = 0;

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
	OTG_ISR_Handler ( &dev );
}

void
usb_wakeup_handler ( void )
{
	printf ( "USB FS wakeup interrupt\n" );
}

void
usb_hs_irq_handler ( void )
{
	// printf ( "HS interrupt\n" );
	OTG_ISR_Handler (&dev);
}

void
usb_hs_ep1_out ( void )
{ 
#ifdef USE_HS
	// printf ( "USB HS ep1 out interrupt\n" );
	(void) OTG_EP1OUT_ISR_Handler ( &dev );
#endif
}
void
usb_hs_ep1_in ( void )
{
#ifdef USE_HS
	// printf ( "USB HS ep1 in interrupt\n" );
	(void) OTG_EP1IN_ISR_Handler ( &dev );
#endif
}

void
usb_hs_wakeup ( void )
{
	printf ( "USB HS wakeup interrupt\n" );
}

/* THE END */
