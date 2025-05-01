/*
 * class.c -- the idea here is to standardize a certain set of calls
 * that every class should expose.  These won't get called directly,
 * but will get called by library/public.c
 *
 * Tom Trebisky  3/23/2025
 */

#include "types.h"
#include "usb_conf.h"

#include "vcp.h"

typedef void (*bfptr) ( char *, int );

void
class_init ( void )
{
		// This is now obsolete since we no longer have these callbacks.
		// usb_register ( &USR_desc, &CDC_cb );
		// usb_register ( NULL, &CDC_cb );
}

void
class_usb_hookup ( bfptr x )
{
		VCP_hookup ( x );
}

void
class_usb_write ( char *buf, int len )
{
	printf ( "class USB write: %d %X, %s\n", len, buf, buf );
	(void) VCP_DataTx ( buf, len );
}

/* This never blocks and returns 0 at a ferrocious rate. */
int
class_usb_read ( char *buf, int len )
{
		return VCPGetBytes ( buf, len );
}

int
class_is_connected ( void )
{
		return VCPGetDTR();
}

/* See descr.c for this: */
// int class_get_descriptor ( int, char **buf, int *len );

/* THE END */
