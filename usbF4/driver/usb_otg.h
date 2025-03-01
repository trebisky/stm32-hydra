/**
  ******************************************************************************
  * @file    usb_otg.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   OTG Core Header
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __USB_OTG__
#define __USB_OTG__

#include "usb_core.h"

void USB_OTG_InitiateSRP(void);
void USB_OTG_InitiateHNP(uint8_t state , uint8_t mode);
void USB_OTG_Switchback (USB_OTG_CORE_HANDLE *pdev);
uint32_t  USB_OTG_GetCurrentState (USB_OTG_CORE_HANDLE *pdev);

uint32_t STM32_USBO_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);

#endif //__USB_OTG__

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

