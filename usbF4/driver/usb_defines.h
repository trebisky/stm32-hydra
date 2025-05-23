/**
  ******************************************************************************
  * @file    usb_defines.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Header of the Core Layer
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __USB_DEF_H__
#define __USB_DEF_H__

#ifdef notdef
/* Moved to usb_conf.h */
#define SPEED_HIGH      0
#define SPEED_FULL      1

#define ULPI_PHY      1
#define EMBEDDED_PHY  2

typedef enum
{
  HS_CORE_ID = 0,
  FS_CORE_ID = 1
} CORE_ID_TypeDef;


enum SPEED {
  USB_SPEED_UNKNOWN = 0,
  USB_SPEED_LOW,
  USB_SPEED_FULL,
  USB_SPEED_HIGH
};

#define EP_TYPE_CTRL                           0
#define EP_TYPE_ISOC                           1
#define EP_TYPE_BULK                           2
#define EP_TYPE_INTR                           3
#define EP_TYPE_MSK                            3

#endif

#define SPEED_PARAM_HIGH 0
#define SPEED_PARAM_HIGH_IN_FULL 1
#define SPEED_PARAM_FULL 3

#define GAHBCFG_TXFEMPTYLVL_EMPTY              1
#define GAHBCFG_TXFEMPTYLVL_HALFEMPTY          0
#define GAHBCFG_GLBINT_ENABLE                  1
#define GAHBCFG_INT_DMA_BURST_SINGLE           0
#define GAHBCFG_INT_DMA_BURST_INCR             1
#define GAHBCFG_INT_DMA_BURST_INCR4            3
#define GAHBCFG_INT_DMA_BURST_INCR8            5
#define GAHBCFG_INT_DMA_BURST_INCR16           7
#define GAHBCFG_DMAENABLE                      1
#define GAHBCFG_TXFEMPTYLVL_EMPTY              1
#define GAHBCFG_TXFEMPTYLVL_HALFEMPTY          0
#define GRXStatus_PKTStatus_IN                       2
#define GRXStatus_PKTStatus_IN_XFER_COMP             3
#define GRXStatus_PKTStatus_DATA_TOGGLE_ERR          5
#define GRXStatus_PKTStatus_CH_HALTED                7

#define MODE_HNP_SRP_CAPABLE                   0
#define MODE_SRP_ONLY_CAPABLE                  1
#define MODE_NO_HNP_SRP_CAPABLE                2
#define MODE_SRP_CAPABLE_DEVICE                3
#define MODE_NO_SRP_CAPABLE_DEVICE             4
#define MODE_SRP_CAPABLE_HOST                  5
#define MODE_NO_SRP_CAPABLE_HOST               6
#define A_HOST                                 1
#define A_SUSPEND                              2
#define A_PERIPHERAL                           3
#define B_PERIPHERAL                           4
#define B_HOST                                 5
#define DEVICE_MODE                            0
#define HOST_MODE                              1
#define OTG_MODE                               2

#define DStatus_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     0
#define DStatus_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     1
#define DStatus_ENUMSPD_LS_PHY_6MHZ               2
#define DStatus_ENUMSPD_FS_PHY_48MHZ              3

#define DCFG_FRAME_INTERVAL_80                 0
#define DCFG_FRAME_INTERVAL_85                 1
#define DCFG_FRAME_INTERVAL_90                 2
#define DCFG_FRAME_INTERVAL_95                 3

#define DEP0CTL_MPS_64                         0
#define DEP0CTL_MPS_32                         1
#define DEP0CTL_MPS_16                         2
#define DEP0CTL_MPS_8                          3

#define EP_SPEED_LOW                           0
#define EP_SPEED_FULL                          1
#define EP_SPEED_HIGH                          2

#define Status_GOUT_NAK                           1
#define Status_DATA_UPDT                          2
#define Status_XFER_COMP                          3
#define Status_SETUP_COMP                         4
#define Status_SETUP_UPDT                         6

#define HC_PID_DATA0                           0
#define HC_PID_DATA2                           1
#define HC_PID_DATA1                           2
#define HC_PID_SETUP                           3

#define HPRT0_PRTSPD_HIGH_SPEED                0
#define HPRT0_PRTSPD_FULL_SPEED                1
#define HPRT0_PRTSPD_LOW_SPEED                 2

#define HCFG_30_60_MHZ                         0
#define HCFG_48_MHZ                            1
#define HCFG_6_MHZ                             2

#define HCCHAR_CTRL                            0
#define HCCHAR_ISOC                            1
#define HCCHAR_BULK                            2
#define HCCHAR_INTR                            3

#define READ_REG32(reg)  (*(__IO uint32_t *)(reg))
#define WRITE_REG32(reg,value) (*(__IO uint32_t *)(reg) = (value))
#define MODIFY_REG32(reg,clear_mask,set_mask) \
  WRITE_REG32((reg), (((READ_REG32(reg)) & ~(clear_mask)) | (set_mask)) )

#endif //__USB_DEFINES__H__

/* THE END */
