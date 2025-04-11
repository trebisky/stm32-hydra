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

#ifndef __H__
#define __H__

/********************************************************************************
EXPORTED FUNCTION FROM THE USB-OTG LAYER
********************************************************************************/
void       Init(HANDLE *pdev ,
                    CORE_ID_TypeDef coreID);

void        DevConnect (HANDLE *pdev);
void        DevDisconnect (HANDLE *pdev);
void        EP_SetAddress (HANDLE *pdev,
                               uint8_t address);
uint32_t    EP_Open(HANDLE *pdev , 
                     uint8_t ep_addr,
                     uint16_t ep_mps,
                     uint8_t ep_type);

uint32_t    EP_Close  (HANDLE *pdev,
                                uint8_t  ep_addr);


uint32_t   EP_PrepareRx ( HANDLE *pdev,
                        uint8_t   ep_addr,                                  
                        uint8_t *pbuf,                                  
                        uint16_t  buf_len);
  
uint32_t    EP_Tx (HANDLE *pdev,
                               uint8_t  ep_addr,
                               uint8_t  *pbuf,
                               uint32_t   buf_len);
uint32_t    EP_Stall (HANDLE *pdev,
                              uint8_t   epnum);
uint32_t    EP_ClrStall (HANDLE *pdev,
                                  uint8_t epnum);
uint32_t    EP_Flush (HANDLE *pdev,
                               uint8_t epnum);
uint32_t    Handle_ISR(HANDLE *pdev);

uint32_t GetEPStatus(HANDLE *pdev ,
                         uint8_t epnum);

void SetEPStatus (HANDLE *pdev , 
                      uint8_t epnum , 
                      uint32_t Status);

#endif //__H__

/* THE END */
