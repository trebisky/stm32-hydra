/*
 * protos.h
 *
 * Ansi prototypes
 * Tom Trebisky 3/30/2025
 */

/* Defined in usbd_req.c */
USBD_Status  USBD_StdDevReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdItfReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdEPReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
void		USBD_ParseSetupRequest (HANDLE  *pdev, USB_SETUP_REQ *req);
void		USBD_CtlError (HANDLE  *pdev, USB_SETUP_REQ *req);


// #include  "usbd_def.h"
// #include  "usbd_core.h"

/* Defined in usbd_ioreq.c */
USBD_Status  USBD_CtlSendData (HANDLE  *pdev, uint8_t *buf, uint16_t len);
USBD_Status  USBD_CtlContinueSendData (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlPrepareRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlContinueRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
USBD_Status  USBD_CtlSendStatus (HANDLE  *pdev);
USBD_Status  USBD_CtlReceiveStatus (HANDLE  *pdev);
// uint16_t  USBD_GetRxCount (HANDLE  *pdev , uint8_t epnum);

/* Defined in core.c */
void USBD_Init(HANDLE *pdev, CORE_ID_TypeDef coreID );
USBD_Status USBD_DeInit(HANDLE *pdev);
USBD_Status USBD_DeInitFull(HANDLE *pdev);
USBD_Status USBD_ClrCfg(HANDLE  *pdev, uint8_t cfgidx);
USBD_Status USBD_SetCfg(HANDLE  *pdev, uint8_t cfgidx);

/* THE END */
