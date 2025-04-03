/* init.c
 * library/init.c - initialization code
 */

#include "types.h"
#include "usb_conf.h"

// #include "usbd_core.h"
#include "usb_core.h"

#include "protos.h"

void
USBD_Init ( USB_OTG_CORE_HANDLE *pdev, USB_OTG_CORE_ID_TypeDef coreID )
{
  /* No harm to call this, but it does nothing */
  // USBD_DeInit(pdev);
  
  /* set USB OTG core params */
  DCD_Init(pdev , coreID);
  
  /* Upon Init call usr callback */
  /* XXX should clean this up */
  STATUS_Reset();
}

USBD_Status
USBD_DeInit ( USB_OTG_CORE_HANDLE *pdev )
{
  return USBD_OK;
}

USBD_Status
USBD_DeInitFull ( USB_OTG_CORE_HANDLE *pdev )
{
  /* Software Init */
  return USBD_OK;
}

/* THE END */

