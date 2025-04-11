/*
 * protos.h
 *
 * Ansi prototypes
 * Tom Trebisky 3/30/2025
 */

/* Defined in usbd_req.c */
Status  StdDevReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
Status  StdItfReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
Status  StdEPReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
void		ParseSetupRequest (HANDLE  *pdev, USB_SETUP_REQ *req);
void		CtlError (HANDLE  *pdev, USB_SETUP_REQ *req);


// #include  "usbd_def.h"
// #include  "usbd_core.h"

/* Defined in usbd_ioreq.c */
Status  CtlSendData (HANDLE  *pdev, uint8_t *buf, uint16_t len);
Status  CtlContinueSendData (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
Status  CtlPrepareRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
Status  CtlContinueRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
Status  CtlSendStatus (HANDLE  *pdev);
Status  CtlReceiveStatus (HANDLE  *pdev);
// uint16_t  GetRxCount (HANDLE  *pdev , uint8_t epnum);

/* Defined in core.c */
void Init(HANDLE *pdev, CORE_ID_TypeDef coreID );
Status DeInit(HANDLE *pdev);
Status DeInitFull(HANDLE *pdev);
Status ClrCfg(HANDLE  *pdev, uint8_t cfgidx);
Status SetCfg(HANDLE  *pdev, uint8_t cfgidx);

/********************* General Driver APIs ********************************************/
/* Defined in driver/driver.c */

// XXX - The following should be static in "driver"
Status  CoreInit        (HANDLE *pdev);
Status  SelectCore      (HANDLE *pdev, CORE_ID_TypeDef coreID);
// Status  EnableGlobalInt (HANDLE *pdev);
// Status  DisableGlobalInt(HANDLE *pdev);
void*           ReadPacket   (HANDLE *pdev , uint8_t *dest, uint16_t len);
Status  WritePacket     (HANDLE *pdev , uint8_t *src, uint8_t ch_ep_num, uint16_t len);
Status  FlushTxFifo     (HANDLE *pdev , uint32_t num);
Status  FlushRxFifo     (HANDLE *pdev);

uint32_t     ReadCoreItr     (HANDLE *pdev);
uint32_t     ReadOtgItr      (HANDLE *pdev);
uint8_t      IsHostMode      (HANDLE *pdev);
uint8_t      IsDeviceMode    (HANDLE *pdev);
uint32_t     GetMode         (HANDLE *pdev);
Status  PhyInit         (HANDLE *pdev);
Status  SetCurrentMode  (HANDLE *pdev, uint8_t mode);

/********************* DEVICE APIs ********************************************/
/* Defined in driver/driver.c */

#ifdef USE_DEVICE_MODE
Status  CoreInitDev         (HANDLE *pdev);
Status  EnableDevInt        (HANDLE *pdev);
uint32_t     ReadDevAllInEPItr           (HANDLE *pdev);
enum SPEED GetDeviceSpeed (HANDLE *pdev);
Status  EP0Activate (HANDLE *pdev);
Status  EPActivate  (HANDLE *pdev , EP *ep);
Status  EPDeactivate(HANDLE *pdev , EP *ep);
Status  EPStartXfer (HANDLE *pdev , EP *ep);
Status  EP0StartXfer(HANDLE *pdev , EP *ep);
Status  EPSetStall          (HANDLE *pdev , EP *ep);
Status  EPClearStall        (HANDLE *pdev , EP *ep);
uint32_t     ReadDevAllOutEp_itr (HANDLE *pdev);
uint32_t     ReadDevOutEP_itr    (HANDLE *pdev , uint8_t epnum);
uint32_t     ReadDevAllInEPItr   (HANDLE *pdev);
void         InitDevSpeed        (HANDLE *pdev , uint8_t speed);
// uint8_t      USBH_IsEvenFrame (HANDLE *pdev);
void         EP0_OutStart(HANDLE *pdev);
void         ActiveRemoteWakeup(HANDLE *pdev);
void         UngateClock(HANDLE *pdev);
void         StopDevice(HANDLE *pdev);
void         DRV_SetEPStatus (HANDLE *pdev , EP *ep , uint32_t Status);
uint32_t     DRV_GetEPStatus(HANDLE *pdev ,EP *ep);
#endif

/* THE END */
