/*
 * protos.h
 *
 * Ansi prototypes
 * Tom Trebisky 3/30/2025
 */

/* Defined in usbd_req.c */
UU_Status  StdDevReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
UU_Status  StdItfReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
UU_Status  StdEPReq (HANDLE  *pdev, USB_SETUP_REQ  *req);
void		ParseSetupRequest (HANDLE  *pdev, USB_SETUP_REQ *req);
void		CtlError (HANDLE  *pdev, USB_SETUP_REQ *req);


// #include  "usbd_def.h"
// #include  "usbd_core.h"

/* Defined in usbd_ioreq.c */
UU_Status  CtlSendData (HANDLE  *pdev, uint8_t *buf, uint16_t len);
UU_Status  CtlContinueSendData (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
UU_Status  CtlPrepareRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
UU_Status  CtlContinueRx (HANDLE  *pdev, uint8_t *pbuf, uint16_t len);
UU_Status  CtlSendStatus (HANDLE  *pdev);
UU_Status  CtlReceiveStatus (HANDLE  *pdev);
// uint16_t  GetRxCount (HANDLE  *pdev , uint8_t epnum);

/* Defined in core.c */
void Init(HANDLE *pdev, CORE_ID_TypeDef coreID );
UU_Status DeInit(HANDLE *pdev);
UU_Status DeInitFull(HANDLE *pdev);
UU_Status ClrCfg(HANDLE  *pdev, uint8_t cfgidx);
UU_Status SetCfg(HANDLE  *pdev, uint8_t cfgidx);

/********************* General Driver APIs ********************************************/
/* Defined in driver/driver.c */

// XXX - The following should be static in "driver"
STS  CoreInit        (HANDLE *pdev);
STS  SelectCore      (HANDLE *pdev, CORE_ID_TypeDef coreID);
// STS  EnableGlobalInt (HANDLE *pdev);
// STS  DisableGlobalInt(HANDLE *pdev);
void*           ReadPacket   (HANDLE *pdev , uint8_t *dest, uint16_t len);
STS  WritePacket     (HANDLE *pdev , uint8_t *src, uint8_t ch_ep_num, uint16_t len);
STS  FlushTxFifo     (HANDLE *pdev , uint32_t num);
STS  FlushRxFifo     (HANDLE *pdev);

uint32_t     ReadCoreItr     (HANDLE *pdev);
uint32_t     ReadOtgItr      (HANDLE *pdev);
uint8_t      IsHostMode      (HANDLE *pdev);
uint8_t      IsDeviceMode    (HANDLE *pdev);
uint32_t     GetMode         (HANDLE *pdev);
STS  PhyInit         (HANDLE *pdev);
STS  SetCurrentMode  (HANDLE *pdev, uint8_t mode);

/********************* DEVICE APIs ********************************************/
/* Defined in driver/driver.c */

#ifdef USE_DEVICE_MODE
STS  CoreInitDev         (HANDLE *pdev);
STS  EnableDevInt        (HANDLE *pdev);
uint32_t     ReadDevAllInEPItr           (HANDLE *pdev);
enum SPEED GetDeviceSpeed (HANDLE *pdev);
STS  EP0Activate (HANDLE *pdev);
STS  EPActivate  (HANDLE *pdev , EP *ep);
STS  EPDeactivate(HANDLE *pdev , EP *ep);
STS  EPStartXfer (HANDLE *pdev , EP *ep);
STS  EP0StartXfer(HANDLE *pdev , EP *ep);
STS  EPSetStall          (HANDLE *pdev , EP *ep);
STS  EPClearStall        (HANDLE *pdev , EP *ep);
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
