/**
  ******************************************************************************
  * @file    usbd_ioreq.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011
  * @brief   header file for the usbd_ioreq.c file
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */ 

#ifndef __USBD_IOREQ_H_
#define __USBD_IOREQ_H_

#include  "usbd_def.h"
#include  "usbd_core.h"

USBD_Status  USBD_CtlSendData (USB_OTG_CORE_HANDLE  *pdev, 
                               uint8_t *buf,
                               uint16_t len);

USBD_Status  USBD_CtlContinueSendData (USB_OTG_CORE_HANDLE  *pdev, 
                               uint8_t *pbuf,
                               uint16_t len);

USBD_Status USBD_CtlPrepareRx (USB_OTG_CORE_HANDLE  *pdev, 
                               uint8_t *pbuf,                                 
                               uint16_t len);

USBD_Status  USBD_CtlContinueRx (USB_OTG_CORE_HANDLE  *pdev, 
                              uint8_t *pbuf,                                          
                              uint16_t len);

USBD_Status  USBD_CtlSendStatus (USB_OTG_CORE_HANDLE  *pdev);

USBD_Status  USBD_CtlReceiveStatus (USB_OTG_CORE_HANDLE  *pdev);

uint16_t  USBD_GetRxCount (USB_OTG_CORE_HANDLE  *pdev , 
                           uint8_t epnum);

#endif /* __USBD_IOREQ_H_ */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
