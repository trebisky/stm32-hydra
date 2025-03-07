/**
  ******************************************************************************
  * @file    usb_bsp.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Specific api's relative to the used hardware platform
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __USB_BSP__H__
#define __USB_BSP__H__

#include "usb_core.h"

void BSP_Init(void);

void USB_OTG_BSP_Init (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_DeInit (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_uDelay (const uint32_t usec);
void USB_OTG_BSP_mDelay (const uint32_t msec);
void USB_OTG_BSP_EnableInterrupt (USB_OTG_CORE_HANDLE *pdev);
void USB_OTG_BSP_DisableInterrupt (USB_OTG_CORE_HANDLE *pdev);
#ifdef USE_HOST_MODE
-- void USB_OTG_BSP_ConfigVBUS(USB_OTG_CORE_HANDLE *pdev);
-- void USB_OTG_BSP_DriveVBUS(USB_OTG_CORE_HANDLE *pdev,uint8_t state);
-- void USB_OTG_BSP_Resume(USB_OTG_CORE_HANDLE *pdev) ;                                                                
-- void USB_OTG_BSP_Suspend(USB_OTG_CORE_HANDLE *pdev);
#endif

#endif //__USB_BSP__H__

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

