/*
 * protos.h
 *
 * Ansi prototypes
 * Tom Trebisky 3/30/2025
 */

/* Defined in usbd_req.c */
USBD_Status  USBD_StdDevReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdItfReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdEPReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
void		USBD_ParseSetupRequest (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);
void		USBD_CtlError (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

// #include  "usbd_def.h"
// #include  "usbd_core.h"

/* Defined in usbd_ioreq.c */
USBD_Status  USBD_CtlSendData (USB_OTG_CORE_HANDLE  *pdev, uint8_t *buf, uint16_t len);
USBD_Status  USBD_CtlContinueSendData (USB_OTG_CORE_HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlPrepareRx (USB_OTG_CORE_HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlContinueRx (USB_OTG_CORE_HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlSendStatus (USB_OTG_CORE_HANDLE  *pdev);
USBD_Status  USBD_CtlReceiveStatus (USB_OTG_CORE_HANDLE  *pdev);
uint16_t  USBD_GetRxCount (USB_OTG_CORE_HANDLE  *pdev , uint8_t epnum);

/* THE END */
