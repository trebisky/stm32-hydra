/**
  ******************************************************************************
  * @file    usbd_usr.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   This file includes the user application layer
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics
  ******************************************************************************
  */ 

#include <library/usbd_usr.h>
#include <library/usbd_ioreq.h>


USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,
  
  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,  
};

enum {
	USB_CONFIGURED = 1,
	USB_CONNECTED = 2,
	USB_RESUMED = 4,
};

/*
enum {
	USB_CONFIGURED = BIT(0),
	USB_CONNECTED = BIT(1),
	USB_RESUMED = BIT(2),
};
*/

static volatile uint8_t usbd_status;

uint8_t usb_isConfigured(void) { return (usbd_status&USB_CONFIGURED); }
#ifdef VBUS_SENSING_ENABLED
uint8_t usb_isConnected(void) { return (usbd_status&USB_CONNECTED); }
#else
uint8_t usb_isConnected(void) { return USB_CONNECTED; }
#endif
uint8_t usb_getStatus(void) { return usbd_status; }

/**
* @brief  USBD_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Init(void)
{   
  /* Setup SysTick Timer for 40 msec interrupts 
  This interrupt is used to probe the joystick */
#if 0
	if (SysTick_Config(SystemCoreClock / 24))
  { 
    /* Capture error */ 
    while (1);
  }
#endif

	usbd_status = 0;
}

/**
* @brief  USBD_USR_DeviceReset 
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_DeviceReset(uint8_t speed )
{
 switch (speed)
 {
   case USB_OTG_SPEED_HIGH: 
     break;

  case USB_OTG_SPEED_FULL: 
     break;
 default:
     break;
     
 }
	usbd_status = 0;
}


/**
* @brief  USBD_USR_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConfigured (void)
{
	usbd_status |= USB_CONFIGURED;
}


/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConnected (void)
{
	usbd_status |= USB_CONNECTED;
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceDisconnected (void)
{
	usbd_status &= ~USB_CONNECTED;
}

/**
* @brief  USBD_USR_DeviceSuspended 
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceSuspended(void)
{
  /* Users can do their application actions here for the USB-Reset */
	usbd_status &= ~USB_RESUMED;
}


/**
* @brief  USBD_USR_DeviceResumed 
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceResumed(void)
{
  /* Users can do their application actions here for the USB-Reset */
	usbd_status |= USB_RESUMED;
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/






























