/*
 * class.c -- the idea here is to standardize a certain set of calls
 * that every class should expose.  These won't get called directly,
 * but will get called by library/public.c
 *
 * Tom Trebisky  3/23/2025
 */
#include "../hydra.h"
#include "hydra_usb.h"
#include "usb.h"

#include "usbd_usr.h"

#include "usbd_cdc_core.h"
#include "usbd_desc.h"

typedef void (*bfptr) ( char *, int );

void
class_init ( void )
{
		// usb_register ( &USR_desc, &USBD_CDC_cb );
		usb_register ( NULL, &USBD_CDC_cb );
}

void
class_usb_hookup ( bfptr x )
{
		VCP_hookup ( x );
}

void
class_usb_write ( char *buf, int len )
{
	(void) VCP_DataTx ( buf, len );
}

/* This never blocks and returns 0 at a ferrocious rate. */
int
class_usb_read ( char *buf, int len )
{
		return VCPGetBytes ( buf, len );
}

/* See descr.c for this: */
// int class_get_descriptor ( int, char **buf, int *len );

/* THE END */
