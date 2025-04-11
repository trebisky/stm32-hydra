/**
  ******************************************************************************
  * @file    usb_dcd.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Peripheral Driver Header file
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __DCD_H__
#define __DCD_H__

/********************************************************************************
EXPORTED FUNCTION FROM THE USB-OTG LAYER
********************************************************************************/
void       DCD_Init(HANDLE *pdev ,
                    CORE_ID_TypeDef coreID);

void        DCD_DevConnect (HANDLE *pdev);
void        DCD_DevDisconnect (HANDLE *pdev);
void        DCD_EP_SetAddress (HANDLE *pdev,
                               uint8_t address);
uint32_t    DCD_EP_Open(HANDLE *pdev , 
                     uint8_t ep_addr,
                     uint16_t ep_mps,
                     uint8_t ep_type);

uint32_t    DCD_EP_Close  (HANDLE *pdev,
                                uint8_t  ep_addr);


uint32_t   DCD_EP_PrepareRx ( HANDLE *pdev,
                        uint8_t   ep_addr,                                  
                        uint8_t *pbuf,                                  
                        uint16_t  buf_len);
  
uint32_t    DCD_EP_Tx (HANDLE *pdev,
                               uint8_t  ep_addr,
                               uint8_t  *pbuf,
                               uint32_t   buf_len);
uint32_t    DCD_EP_Stall (HANDLE *pdev,
                              uint8_t   epnum);
uint32_t    DCD_EP_ClrStall (HANDLE *pdev,
                                  uint8_t epnum);
uint32_t    DCD_EP_Flush (HANDLE *pdev,
                               uint8_t epnum);
uint32_t    DCD_Handle_ISR(HANDLE *pdev);

uint32_t DCD_GetEPStatus(HANDLE *pdev ,
                         uint8_t epnum);

void DCD_SetEPStatus (HANDLE *pdev , 
                      uint8_t epnum , 
                      uint32_t Status);

#endif //__DCD_H__

/* THE END */
