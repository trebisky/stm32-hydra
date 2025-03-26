/* --------------------
 * public.c
 * Tom Trebisky  3-23-2025
 */
#include "../hydra.h"
#include "hydra_usb.h"
#include "usb.h"

#include <stdarg.h>

#include "usbd_usr.h"

/* XXX - for now we need these include files,
 * but ultimately we will have the class register these
 * things in response to a init call
 */
// library/public.c:124:14: error: 'USR_desc' undeclared (first use in this function)
// library/public.c:125:14: error: 'USBD_CDC_cb' undeclared (first use in this function)
// #include "vcp/usbd_cdc_core.h"
// #include "vcp/usbd_desc.h"

typedef void (*bfptr) ( char *, int );

static void gpio_usb_init ( void );

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

/* From -- vcp/usbd_desc.c */
static USBD_DEVICE *class_desc;

/* From -- vcp/usbd_cdc_core.c */
// static USBD_Class_cb_TypeDef  USBD_CDC_cb;
static USBD_Class_cb_TypeDef  *class_cb;

/* From -- library/usbd_usr.c */
// static USBD_Usr_cb_TypeDef USR_cb;

/* Called by the class in response to the class_init() call
 */
void
usb_register ( USBD_DEVICE *desc, USBD_Class_cb_TypeDef *cb )
{
		class_desc = desc;
		class_cb = cb;
}

/* ============================================================================== */
/* ============================================================================== */
/* Next we have things called from the above that are "glue" to the
* private routines in the USB code.
*/

USB_OTG_CORE_HANDLE  USB_OTG_dev;

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

        gpio_usb_init ();

		/* Should trigger a callback to usb_register() */
		class_init ();

/* Change this choice in usb_conf.h */
#ifdef USE_USB_OTG_HS
	  printf ( "Initialize HS usb core with IRQ %d\n", IRQ_USB_HS );
      USBD_Init(&USB_OTG_dev,
            USB_OTG_HS_CORE_ID,
            class_desc,
            class_cb,
            &USR_cb);
#else
	  printf ( "Initialize FS usb core with IRQ %d\n", IRQ_USB_FS );
      USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            class_desc,
            class_cb,
            &USR_cb);
#endif

#ifdef notdef
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

#endif
		/* XXX someday will be 0 or 1 */
		return 0;
}

#ifdef notdef
void
usbPowerOff ( void )
{
	USBD_DeInitFull(&USB_OTG_dev);
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

#ifdef notdef
/* XXX get rid of these, they are only correct for
 * the CPU clock on the F411
 */
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
#endif

/* A footnote on figure 26, page 272 of the TRM says
 * that the value 12 is used for the usb HS when used in
 * FS mode.
 */
#define GPIO_ALT_USB	10
#define GPIO_ALT_USB_FS	12

/* This is for the F411 and/or F429, F407 */
static void
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
// static int usb_debug_mask = DM_DESC;
static int usb_debug_mask = DM_ALL;
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
	USBD_OTG_ISR_Handler ( &USB_OTG_dev );
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
