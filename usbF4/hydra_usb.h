/* Tom Trebisky (c) 3-6-2025
 *
 * hydra_usb.h
 */

void usb_debug ( int, char *, ... );
void usb_dump ( int, char *, char *, int );

/* debug selectors --
 * These are bits in a mask, so we get 32 possibilities.
 */

#define DM_ORIG		1
#define DM_EVENT	2
#define DM_ENUM		4

/* THE END */
