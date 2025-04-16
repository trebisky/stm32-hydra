/**
  ******************************************************************************
  * @file    usb_conf.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   General low level driver configuration
  ******************************************************************************
  * COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#ifndef __USB_CONF__H__
#define __USB_CONF__H__

/* Tom Trebisky (c) 3-6-2025
 */
void usb_debug ( int, char *, ... );
void usb_dump ( int, char *, char *, int );

/* debug selectors --
 *  first argument to the above.
 * These are bits in a mask, so we get 32 possibilities.
 */

#define DM_ORIG		1
#define DM_EVENT	2
#define DM_ENUM		4
#define DM_WRITE1	8		/* writes to endpoint */
#define DM_READ1	0x10	/* reads from endpoint */
#define DM_DESC		0x20	/* descriptors for enumeration */
#define DM_ALL		0xffffffff

/* ---------------------------------------------------------------------------- */

/* These may change with a different class, this is correct for VCP */

#define CFG_MAX_NUM                1
#define ITF_MAX_NUM                1

/* ---------------------------------------------------------------------------- */

/* USB Core and PHY interface configuration.
   Tip: To avoid modifying these defines each time you need to change the USB
        configuration, you can declare the needed define in your toolchain
        compiler preprocessor.
   */

// tjt 3-8-2025
#define USE_EMBEDDED_PHY
#define EMBEDDED_PHY_ENABLED

// #define USE_FS
#define USE_HS

#define USE_DEVICE_MODE
//#define USE_HOST_MODE
//#define USE_OTG_MODE

#ifdef USE_FS
 #define FS_CORE
#else
 #define HS_CORE
#endif

/* ---------------------------------------------------------------------------- */

#ifndef FS_CORE
 #ifndef HS_CORE
    #error  "HS_CORE or FS_CORE should be defined"
 #endif
#endif


#ifndef USE_DEVICE_MODE
 #ifndef USE_HOST_MODE
    #error  "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
 #endif
#endif

#ifndef USE_HS
 #ifndef USE_FS
    #error  "USE_HS or USE_FS should be defined"
 #endif
#endif

#ifdef USE_HS
 #ifndef USE_ULPI_PHY
  #ifndef USE_EMBEDDED_PHY
   #ifndef USE_I2C_PHY
     #error  "USE_ULPI_PHY or USE_EMBEDDED_PHY or USE_I2C_PHY should be defined"
   #endif
  #endif
 #endif
#endif

/*******************************************************************************
*                      FIFO Size Configuration in Device mode
*
*  (i) Receive data FIFO size = RAM for setup packets +
*                   OUT endpoint control information +
*                   data OUT packets + miscellaneous
*      Space = ONE 32-bits words
*     --> RAM for setup packets = 10 spaces
*        (n is the nbr of CTRL EPs the device core supports)
*     --> OUT EP CTRL info      = 1 space
*        (one space for status information written to the FIFO along with each
*        received packet)
*     --> data OUT packets      = (Largest Packet Size / 4) + 1 spaces
*        (MINIMUM to receive packets)
*     --> OR data OUT packets  = at least 2*(Largest Packet Size / 4) + 1 spaces
*        (if high-bandwidth EP is enabled or multiple isochronous EPs)
*     --> miscellaneous = 1 space per OUT EP
*        (one space for transfer complete status information also pushed to the
*        FIFO with each endpoint's last packet)
*
*  (ii)MINIMUM RAM space required for each IN EP Tx FIFO = MAX packet size for
*       that particular IN EP. More space allocated in the IN EP Tx FIFO results
*       in a better performance on the USB and can hide latencies on the AHB.
*
*  (iii) TXn min size = 16 words. (n  : Transmit FIFO index)
*   (iv) When a TxFIFO is not used, the Configuration should be as follows:
*       case 1 :  n > m    and Txn is not used    (n,m  : Transmit FIFO indexes)
*       --> Txm can use the space allocated for Txn.
*       case2  :  n < m    and Txn is not used    (n,m  : Transmit FIFO indexes)
*       --> Txn should be configured with the minimum space of 16 words
*  (v) The FIFO is used optimally when used TxFIFOs are allocated in the top
*       of the FIFO.Ex: use EP1 and EP2 as IN instead of EP1 and EP3 as IN ones.
*******************************************************************************/



/****************** USB OTG FS CONFIGURATION **********************************/
#ifdef FS_CORE
 #define RX_FIFO_FS_SIZE                          128
 #define TX0_FIFO_FS_SIZE                          64
 #define TX1_FIFO_FS_SIZE                         128
 #define TX2_FIFO_FS_SIZE                          0
 #define TX3_FIFO_FS_SIZE                          0

 //#define FS_LOW_PWR_MGMT_SUPPORT
 //#define FS_SOF_OUTPUT_ENABLED
#endif

/****************** USB OTG HS CONFIGURATION **********************************/
#ifdef HS_CORE
 #define RX_FIFO_HS_SIZE                          512
 #define TX0_FIFO_HS_SIZE                         512
 #define TX1_FIFO_HS_SIZE                         512
 #define TX2_FIFO_HS_SIZE                          0
 #define TX3_FIFO_HS_SIZE                          0
 #define TX4_FIFO_HS_SIZE                          0
 #define TX5_FIFO_HS_SIZE                          0
 #define TXH_NP_HS_FIFOSIZ                         96
 #define TXH_P_HS_FIFOSIZ                          96

#define HS_INTERNAL_DMA_ENABLED
#define HS_DEDICATED_EP1_ENABLED

//#define HS_LOW_PWR_MGMT_SUPPORT
//#define HS_SOF_OUTPUT_ENABLED

//#define INTERNAL_VBUS_ENABLED
#define EXTERNAL_VBUS_ENABLED

#ifdef USE_ULPI_PHY
  #define ULPI_PHY_ENABLED
#endif

#ifdef USE_EMBEDDED_PHY
   #define EMBEDDED_PHY_ENABLED
#endif

#endif /* HS_CORE */

/* In HS mode and when the DMA is used, all variables and data structures dealing
   with the DMA during the transaction process should be 4-bytes aligned */

#ifdef HS_INTERNAL_DMA_ENABLED
  #define __ALIGN_BEGIN
  #define __ALIGN_END    __attribute__ ((aligned (4)))
#else
  #define __ALIGN_BEGIN
  #define __ALIGN_END    __attribute__ ((aligned (4)))
#endif /* HS_INTERNAL_DMA_ENABLED */

/* I only expect to ever use gcc */
#ifndef __packed
  #define __packed    __attribute__ ((__packed__))
#endif

#endif //__USB_CONF__H__

/* THE END */
