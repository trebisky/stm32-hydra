/**
  ******************************************************************************
  * @file    usb_core.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    22-July-2011
  * @brief   Header of the Core Layer
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __USB_CORE_H__
#define __USB_CORE_H__

#define  MIN(a, b)      (((a) < (b)) ? (a) : (b))

/* Also in usb_regs.h */
#define MAX_TX_FIFOS                 7
#define MAX_EP0_SIZE                 64

/* Standard USB, belongs is usb_std.h someday */
typedef  struct  usb_setup_req {
    uint8_t   bmRequest;                      
    uint8_t   bRequest;                           
    uint16_t  wValue;                             
    uint16_t  wIndex;                             
    uint16_t  wLength;                            
} USB_SETUP_REQ;

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

/* Why both this and the above XXX ?*/
#define SPEED_HIGH      0
#define SPEED_FULL      1

/* Which PHY we are using */
#define ULPI_PHY      1
#define EMBEDDED_PHY  2

typedef enum {
  UU_OK   = 0,
  UU_BUSY,
  UU_FAIL,
} UU_Status;

/* Endpoint types */
#define EP_CONTROL                       0
#define EP_ISOC                          1
#define EP_BULK                          2
#define EP_INT                           3
#define EP_MASK                          3

/* More silly redundandy to clean up XXX */
#define EP_TYPE_CTRL                           0
#define EP_TYPE_ISOC                           1
#define EP_TYPE_BULK                           2
#define EP_TYPE_INTR                           3
#define EP_TYPE_MSK                            3

/* Thus used to be in device/usb_cdc.h, but it really belongs here */
/*  Device Status */
#define DEFAULT                          1
#define ADDRESSED                        2
#define CONFIGURED                       3
#define SUSPENDED                        4

#define EP0_IDLE                          0
#define EP0_SETUP                         1
#define EP0_DATA_IN                       2
#define EP0_DATA_OUT                      3
#define EP0_STATUS_IN                     4
#define EP0_STATUS_OUT                    5
#define EP0_STALL                         6

#define EP_TX_DIS       0x0000
#define EP_TX_STALL     0x0010
#define EP_TX_NAK       0x0020
#define EP_TX_VALID     0x0030
 
#define EP_RX_DIS       0x0000
#define EP_RX_STALL     0x1000
#define EP_RX_NAK       0x2000
#define EP_RX_VALID     0x3000

#define   MAX_DATA_LENGTH                        0x100

typedef enum {
  OK = 0,
  FAIL
} STS;

typedef enum {
  HC_IDLE = 0,
  HC_XFRC,
  HC_HALTED,
  HC_NAK,
  HC_NYET,
  HC_STALL,
  HC_XACTERR,  
  HC_BBLERR,   
  HC_DATATGLERR,  
} HC_STATUS;

typedef enum {
  URB_IDLE = 0,
  URB_DONE,
  URB_NOTREADY,
  URB_ERROR,
  URB_STALL
} URB_STATE;

typedef enum {
  CTRL_START = 0,
  CTRL_XFRC,
  CTRL_HALTED,
  CTRL_NAK,
  CTRL_STALL,
  CTRL_XACTERR,  
  CTRL_BBLERR,   
  CTRL_DATATGLERR,  
  CTRL_FAIL
} CTRL_STATUS;

typedef struct ep
{
  uint8_t        num;
  uint8_t        is_in;
  uint8_t        is_stall;  
  uint8_t        type;
  uint8_t        data_pid_start;
  uint8_t        even_odd_frame;
  uint16_t       tx_fifo_num;
  uint32_t       maxpacket;
  /* transaction level variables*/
  uint8_t        *xfer_buff;
  uint32_t       dma_addr;  
  uint32_t       xfer_len;
  uint32_t       xfer_count;
  /* Transfer level variables*/  
  uint32_t       rem_data_len;
  uint32_t       total_data_len;
  uint32_t       ctl_data_len;  
}

EP , *PEP;


typedef struct core_cfg
{
  uint8_t       host_channels;
  uint8_t       dev_endpoints;
  uint8_t       speed;
  uint8_t       dma_enable;
  uint16_t      mps;
  uint16_t      TotalFifoSize;
  uint8_t       phy_itface;
  uint8_t       Sof_output;
  uint8_t       low_power;
  uint8_t       coreID;
 
}
CORE_CFGS, *PCORE_CFGS;

struct _DCD
{
  uint8_t        device_config;
  uint8_t        device_state;
  uint8_t        device_status;
  uint8_t        device_old_status;
  uint8_t        device_address;
  uint8_t        connection_status;  
  uint8_t        test_mode;
  uint32_t       DevRemoteWakeup;
  EP     in_ep   [MAX_TX_FIFOS];
  EP     out_ep  [MAX_TX_FIFOS];
  uint8_t        setup_packet [8*3];
  // Class_cb_TypeDef         *class_cb;
  // Usr_cb_TypeDef           *usr_cb;
  // DEVICE                   *usr_device;  
  uint8_t        *pConfig_descriptor;		/* used by VCP */
};

typedef struct _DCD DEV;
// typedef struct _DCD *PDEV;

struct handle
{
  // struct pickle *pptr;
  CORE_CFGS    cfg;
  // CORE_REGS    regs;
  struct core_regs *hw;
#ifdef USE_DEVICE_MODE
  DEV     dev;
#endif

#ifdef USE_HOST_MODE
..   HCD_DEV     host;
#endif
#ifdef USE_OTG_MODE
  /* tjt - we don't define this */
  OTG_DEV     otg;
#endif
};

typedef struct handle HANDLE;
// typedef struct handle *PHANDLE;

#ifdef USE_OTG_MODE
/* Not used */
typedef struct _OTG
{
  uint8_t    OTG_State;
  uint8_t    OTG_PrevState;  
  uint8_t    OTG_Mode;    
}
OTG_DEV , *USBO_PDEV;
#endif

#endif  /* __USB_CORE_H__ */

/* THE END */
