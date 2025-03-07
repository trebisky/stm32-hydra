/**
  ******************************************************************************
  * @file    usb_bsp.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   This file is responsible to offer board support package and is
  *          configurable by user.
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */

#include <driver/usb_bsp.h>
#include "usbd_conf.h"
// #include <libmaple/gpio.h>

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

#define OTG_FS_IRQn 67
//typedef unsigned char uint8_t;
#include "misc.h"

#ifndef HYDRA
/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
* @retval None
*/

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
	// ala42
#define GPIO_AF_OTG1_FS         ((uint8_t)0xA)  /* OTG_FS Alternate Function mapping */
	gpio_set_mode(BOARD_USB_DM_PIN,GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_OSPEED_100MHZ);
	gpio_set_mode(BOARD_USB_DP_PIN,GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_OSPEED_100MHZ);
	gpio_set_af_mode(BOARD_USB_DM_PIN,GPIO_AF_OTG1_FS) ;	// OTG_FS_DM
	gpio_set_af_mode(BOARD_USB_DP_PIN,GPIO_AF_OTG1_FS) ;	// OTG_FS_DP
#ifdef USB_OTG_FS_SOF_OUTPUT_ENABLED
	gpio_set_mode(GPIOA, 8,GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_OSPEED_100MHZ);
	gpio_set_af_mode(GPIOA, 8,GPIO_AF_OTG1_FS) ;		// OTG_FS_SOF
#endif
#ifdef VBUS_SENSING_ENABLED
	gpio_set_mode(GPIOA, 9,GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_OSPEED_100MHZ);
	gpio_set_af_mode(GPIOA, 9,GPIO_AF_OTG1_FS) ;		// OTG_FS_VBUS
#endif

#ifdef ID_SENSING_ENABLED
	gpio_set_mode(GPIOA,10,GPIO_MODE_OUTPUT | GPIO_OTYPE_OD | GPIO_PUPD_INPUT_PU | GPIO_OSPEED_100MHZ);
	gpio_set_af_mode(GPIOA,10,GPIO_AF_OTG1_FS) ;	// OTG_FS_ID
#endif
	rcc_clk_enable(RCC_SYSCFG);
	rcc_clk_enable(RCC_USBFS);
}

void USB_OTG_BSP_DeInit(USB_OTG_CORE_HANDLE *pdev)
{
	// ala42
#define GPIO_AF0         ((uint8_t)0)  /* OTG_FS Alternate Function mapping */
	gpio_set_mode(BOARD_USB_DM_PIN, GPIO_MODE_INPUT);
	gpio_set_mode(BOARD_USB_DP_PIN, GPIO_MODE_INPUT);
	gpio_set_af_mode(BOARD_USB_DM_PIN,GPIO_AF0) ;	// OTG_FS_DM
	gpio_set_af_mode(BOARD_USB_DP_PIN,GPIO_AF0) ;	// OTG_FS_DP
#ifdef USB_OTG_FS_SOF_OUTPUT_ENABLED
	gpio_set_mode(GPIOA, 8,GPIO_MODE_INPUT);
	gpio_set_af_mode(GPIOA, 8,GPIO_AF0) ;		// OTG_FS_SOF
#endif
#ifdef VBUS_SENSING_ENABLED
	gpio_set_mode(GPIOA, 9,GPIO_MODE_INPUT);
	gpio_set_af_mode(GPIOA, 9,GPIO_AF0) ;		// OTG_FS_VBUS
#endif

#ifdef ID_SENSING_ENABLED
	gpio_set_mode(GPIOA,10,GPIO_MODE_INPUT);
	gpio_set_af_mode(GPIOA,10,GPIO_AF0) ;	// OTG_FS_ID
#endif

//	rcc_clk_disable(RCC_SYSCFG);
	rcc_clk_disable(RCC_USBFS);
}


/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global interrupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
#if 1
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
#ifdef USE_USB_OTG_HS
--  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
#else
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
#endif
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
--  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
--  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_OUT_IRQn;
--  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
--  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
--  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
--  NVIC_Init(&NVIC_InitStructure);

--  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
--  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_IN_IRQn;
--  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
--  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
--  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
--  NVIC_Init(&NVIC_InitStructure);
#endif
#endif
}

void USB_OTG_BSP_DisableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
#if 1
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
#ifdef USE_USB_OTG_HS
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
#else
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
#endif
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
  NVIC_Init(&NVIC_InitStructure);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_OUT_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_IN_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif
#endif
}

#endif	/* Hydra */

/**
* @brief  USB_OTG_BSP_uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
* @retval None
*/
void USB_OTG_BSP_uDelay (const uint32_t usec)
{
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if ( ++count > utime )
    {
      return ;
    }
  }
  while (1);
}


/**
* @brief  USB_OTG_BSP_mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void USB_OTG_BSP_mDelay (const uint32_t msec)
{
  USB_OTG_BSP_uDelay(msec * 1000);
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
