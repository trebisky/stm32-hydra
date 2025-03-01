/**
  ******************************************************************************
  * @file    usbd_req.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-July-2011
  * @brief   header file for the usbd_req.c file
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */ 

#ifndef __USB_REQUEST_H_
#define __USB_REQUEST_H_

#include  "usbd_def.h"
#include  "usbd_core.h"
#include  <vcp/usbd_conf.h>

USBD_Status  USBD_StdDevReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdItfReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
USBD_Status  USBD_StdEPReq (USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ  *req);
void USBD_ParseSetupRequest( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

void USBD_CtlError( USB_OTG_CORE_HANDLE  *pdev, USB_SETUP_REQ *req);

void USBD_GetString(uint8_t *desc, uint8_t *unicode, uint16_t *len);

#endif /* __USB_REQUEST_H_ */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
