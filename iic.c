/*
 * Copyright (C) 2016  Tom Trebisky  <tom@mmto.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 */

/* Tom Trebisky
 * 5-11-2016 - begun for ESP8266
 * 5-29-2016 - begin port for ARM/BBB/Kyu
 * 6-22-2016 - integrated under new i2c.c
 * 11-13-2020 - moved to libmaple-unwired
 *
 *  iic.c
 *
 *  Bit banging i2c library
 *    for the ESP8266 and for Kyu/BBB
 *
 * A key idea is that the sda and scl pins can be
 * specified as arguments to the initializer function.
 *
 * A fairly high level interface is presented as an API.
 *  low level code derived from i2c_master.c
 *
 * There are 3 functions in the API:
 *    void iic_init ( sda_pin, sclk_pin );
 *    int iic_send ( addr, unsigned char *, int );
 *    int iic_recv ( addr, unsigned char *, int );
 */
// #include <kyu.h>

/* For libmaple-unwired */
// #define ARCH_MAPLE
// #define ARCH_ARM

/* For Kyu */
// #define ARCH_KYU
// #define ARCH_ARM

/* For Hydra */
#define ARCH_HYDRA
#define ARCH_ARM

//#include "serial.h"
//#include "time.h"
//#include "gpio.h"
//#include "io.h"

#ifdef ARCH_HYDRA
#define GPIO_INPUT(g,p)		gpio_input_config ( g, p )
#define GPIO_OUTPUT(g,p)	gpio_output_od_config ( g, p )
#define GPIO_READ(g,p)		gpio_read ( g, p )

// #define GPIO_SET(g,p)	gpio_bit ( g, p, 1 )
// #define GPIO_CLEAR(g,p)	gpio_bit ( g, p, 0 )
#define GPIO_VAL(g,p,x)	gpio_bit ( g, p, x )
#endif

#ifdef ARCH_MAPLE
#define GPIO_INPUT(x)	pinMode ( x, INPUT_FLOATING )
#define GPIO_OUTPUT(x)	pinMode ( x, OUTPUT_OPEN_DRAIN )
#define GPIO_READ(x)	digitalRead ( x )

#define GPIO_SET(x)	digitalWrite ( x, 1 )
#define GPIO_CLEAR(x)	digitalWrite ( x, 0 )
#endif

/* On the Maple/STM32, we have a handy upper layer on the more fundamental
 * lower gpio layer.  The lower layer requires a gpio device and pin.
 * The upper layer requires a single index that indexes pinmap transparently
 * hiding the details of device/pin.  So we use the upper layer.
 */

#ifdef ARCH_ESP8266
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#define printf	os_printf
#endif

#ifdef ARCH_ARM
#ifdef ARCH_KYU
#include "gpio.h"
#define GPIO_READ(x)	gpio_read_bit ( x )
#define GPIO_INPUT(x)	gpio_dir_in ( x )
#define GPIO_OUTPUT(x)	gpio_dir_out ( x )
#define GPIO_SET(x)	gpio_set_bit ( x )
#define GPIO_CLEAR(x)	gpio_clear_bit ( x )
#endif
#endif

static void iic_setdc ( int, int );
static void iic_dc ( int, int );
static void iic_dc_wait ( int, int, int );
static void iic_writeb ( int );
static int iic_readb ( void );
static void iic_setAck ( int );
static int iic_getAck ( void );

static void iic_start ( void );
static void iic_stop ( void );
static int iic_recv_byte ( int );
static int iic_send_byte ( int );

/* The following functions are all that a person
 * should ever need to use.
 */
// void iic_init ( int, int );
void iic_init ( int, int, int, int );
int iic_send ( int, unsigned char *, int );
int iic_recv ( int, unsigned char *, int );

#ifndef ARCH_ESP8266
#define ICACHE_FLASH_ATTR
typedef unsigned char uint8;

#define os_delay_us(x)	delay_us ( (x) )
#endif

/* -------------------------------------------------- */

#ifdef ARCH_ESP8266
/* Part of a general GPIO facility. */

/* place holder for unused channels */
#define Z       0

/* This is an array of pin control register addresses.
 */
static const int mux[] = {
    PERIPHS_IO_MUX_GPIO0_U,     /* 0 - D3 */
    PERIPHS_IO_MUX_U0TXD_U,     /* 1 - uart */
    PERIPHS_IO_MUX_GPIO2_U,     /* 2 - D4 */
    PERIPHS_IO_MUX_U0RXD_U,     /* 3 - uart */
    PERIPHS_IO_MUX_GPIO4_U,     /* 4 - D2 */
    PERIPHS_IO_MUX_GPIO5_U,     /* 5 - D1 */
    Z,  /* 6 */
    Z,  /* 7 */
    Z,  /* 8 */
    PERIPHS_IO_MUX_SD_DATA2_U,  /* 9   - D11 (SD2) */
    PERIPHS_IO_MUX_SD_DATA3_U,  /* 10  - D12 (SD3) */
    Z,  /* 11 */
    PERIPHS_IO_MUX_MTDI_U,      /* 12 - D6 */
    PERIPHS_IO_MUX_MTCK_U,      /* 13 - D7 */
    PERIPHS_IO_MUX_MTMS_U,      /* 14 - D5 */
    PERIPHS_IO_MUX_MTDO_U       /* 15 - D8 */
};

/* These are the mux values that put a pin into GPIO mode
 */
static const uint8 func[] = { 0, 3, 0, 3,   0, 0, Z, Z,   Z, 3, 3, Z,   3, 3, 3, 3 };

/* ESP8266 */
static void ICACHE_FLASH_ATTR
gpio_iic_setup ( int gpio )
{
    int reg;

    PIN_FUNC_SELECT ( mux[gpio], func[gpio] );

    /* make this open drain */
    reg = GPIO_PIN_ADDR ( gpio );
    GPIO_REG_WRITE ( reg, GPIO_REG_READ( reg ) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE) );

    GPIO_REG_WRITE (GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << gpio) );
}
#endif

/* -------------------------------------------------- */

/* Some notes on the i2c protocol
 * both signals are open drain.
 *  a device can pull a line low, but can never drive it high.
 * an idle bus has both sda and scl high
 * a start sequence pulls SDA low, with SCL left high
 * a stop sequence first raises SCL, then raises SDA
 *  (avoid changing SDS with SCL high to avoid false stops)
 * data is asserted after SCL falls, is sampled when SCL rises.
 */
/* -------------------------------------------------- */

/* For Hydra */
static uint8 sda_gpio;
static uint8 scl_gpio;

static uint8 sda_pin;
static uint8 scl_pin;

static uint8 cur_sda;
static uint8 cur_scl;

#ifdef ARCH_ESP8266
static uint8 sda_mask;
static uint8 scl_mask;
static uint8 all_mask;
#endif

#define MAX_BITS	28

static void
iic_bus_init ( void )
{
    int i;

    iic_dc ( 1, 0 );

    iic_dc ( 0, 0 );
    iic_dc ( 1, 0 );

    for (i = 0; i < MAX_BITS; i++) {
	iic_dc ( 1, 0 );
	iic_dc ( 1, 1 );
    }

    iic_stop();
}

#ifdef notdef
/* This does whatever needs to be done to get the gpio
 * system into a state that lets us do what we need to do.
 */
static void ICACHE_FLASH_ATTR
iic_gpio_init ( int sda, int scl )
{
    sda_pin = sda;
    scl_pin = scl;

#ifdef ARCH_ESP8266
    sda_mask = 1 << sda;
    scl_mask = 1 << scl;
    all_mask = sda_mask | scl_mask;

    ETS_GPIO_INTR_DISABLE() ;
    gpio_iic_setup ( sda );
    gpio_iic_setup ( scl );

    ETS_GPIO_INTR_ENABLE() ;
#endif

/* On the BBB this does 2 things:
 * -- sets the pinmux as per:
 *   psp[pin] = MODE(7) | RXACTIVE | SLEWCTRL;
 * -- set the GPIO direction to output.
 */
#ifdef ARCH_ARM
#ifdef ARCH_KYU
    gpio_iic_init ( sda );
    gpio_iic_init ( scl );
#endif
#endif

#ifdef ARCH_MAPLE
    GPIO_OUTPUT ( sda );
    GPIO_OUTPUT ( scl );
#endif

    iic_setdc ( 1, 1 );
}
#endif

/* This does whatever needs to be done to get the gpio
 * system into a state that lets us do what we need to do.
 * Hydra must take note of both gpio and pin for each
 */
void 
iic_init ( int sda_g, int sda_p, int scl_g, int scl_p )
{
    sda_gpio = sda_g;
    sda_pin = sda_p;
    scl_gpio = scl_g;
    scl_pin = scl_p;

    gpio_output_od_config ( sda_gpio, sda_pin );
    gpio_output_od_config ( scl_gpio, scl_pin );

    iic_setdc ( 1, 1 );
    iic_bus_init ();
}

/* -------------------------------------------------- */

#ifdef ARCH_ESP8266
/* Could be a macro */
static int ICACHE_FLASH_ATTR
iic_get_bit ( void )
{
    // return GPIO_INPUT_GET ( SDA_GPIO );
    return ( gpio_input_get() >> sda_pin) & 1;
}

/* ESP8266 */
static void ICACHE_FLASH_ATTR
iic_setdc ( int sda, int scl )
{
    int high_mask;
    int low_mask;

    cur_sda = sda;
    cur_scl = scl;

    if ( sda ) {
        high_mask = sda_mask;
        low_mask = 0;
    } else {
        high_mask = 0;
        low_mask = sda_mask;
    }

    if ( scl )
        high_mask |= scl_mask;
    else
        low_mask |= scl_mask;

    gpio_output_set( high_mask, low_mask, all_mask, 0);
}
#endif

/* These do not require 0/1 return values from GPIO_READ */
#ifdef ARCH_ARM
static int
iic_raw_bit ( void )
{
    // return GPIO_READ ( sda_pin ) ? 1 : 0;
    return GPIO_READ ( sda_gpio, sda_pin ) ? 1 : 0;
}

static int
iic_get_bit ( void )
{
    int rv;

    // GPIO_INPUT ( sda_pin );
    // rv = GPIO_READ ( sda_pin );
    // GPIO_OUTPUT ( sda_pin );
    GPIO_INPUT ( sda_gpio, sda_pin );
    rv = GPIO_READ ( sda_gpio, sda_pin );
    GPIO_OUTPUT ( sda_gpio, sda_pin );
    return rv ? 1 : 0;
}

static void
iic_setdc ( int sda, int scl )
{
    cur_sda = sda;
    GPIO_VAL ( sda_gpio, sda_pin, sda );
    /*
    if ( sda ) {
	GPIO_SET ( sda_pin );
    } else {
	GPIO_CLEAR ( sda_pin );
    }
    */

    cur_scl = scl;
    GPIO_VAL ( scl_gpio, scl_pin, scl );
    /*
    if ( scl ) {
	GPIO_SET ( scl_pin );
    } else {
	GPIO_CLEAR ( scl_pin );
    }
    */
}

static void
iic_setclk ( int scl )
{
    GPIO_VAL ( scl_gpio, scl_pin, scl );
    /*
    if ( scl ) {
	GPIO_SET ( scl_pin );
    } else {
	GPIO_CLEAR ( scl_pin );
    }
    */
}
#endif

static void ICACHE_FLASH_ATTR
iic_dc ( int data, int clock )
{
    iic_setdc ( data, clock );
    os_delay_us ( 5 );
}

static void ICACHE_FLASH_ATTR
iic_dc_wait ( int data, int clock, int wait )
{
    iic_setdc ( data, clock );
    os_delay_us ( wait );
}

static void ICACHE_FLASH_ATTR
iic_start(void)
{
    iic_dc ( 1, cur_scl );
    iic_dc ( 1, 1 );
    iic_dc ( 0, 1 );
}

static void ICACHE_FLASH_ATTR
iic_stop(void)
{
    os_delay_us (5);

    iic_dc ( 0, cur_scl );
    iic_dc ( 0, 1 );
    iic_dc ( 1, 1 );
}

static void ICACHE_FLASH_ATTR
iic_setAck ( int level )
{
    iic_dc ( cur_sda, 0 );
    iic_dc ( level, 0 );
    iic_dc ( level, 1 );
    iic_dc ( level, 0 );
    iic_dc ( 1, 0 );
}

static int ICACHE_FLASH_ATTR
iic_getAck ( void )
{
    int rv;

    iic_dc ( cur_sda, 0 );
    iic_dc ( 1, 0 );
    iic_dc ( 1, 1 );

    rv = iic_get_bit ();
    os_delay_us (5);

    iic_dc ( 1, 0 );

    return rv;
}

#ifdef ARCH_ARM
/* XXX - why even manipulate the sda pin when reading ?? */
/* We send the Ack outside of this routine */
static int
iic_readb ( void )
{
    int rv = 0;
    int i;

    // GPIO_INPUT ( sda_pin );
    GPIO_INPUT ( sda_gpio, sda_pin );

    os_delay_us (5);

    iic_dc ( cur_sda, 0 );

    for (i = 0; i < 8; i++) {
        os_delay_us (5);
	iic_dc ( 1, 0 );
	iic_dc ( 1, 1 );

        rv |= iic_get_bit() << (7-i);

	iic_dc_wait ( 1, 1, i == 7 ? 8 : 5 );
    }

    // GPIO_OUTPUT ( sda_pin );
    GPIO_OUTPUT ( sda_gpio, sda_pin );

    iic_dc ( 1, 0 );

    return rv;
}

/* ARM */
static void
iic_watch ( int delay )
{
    int i;
    int val;

    for ( i=0; i<delay; i++ ) {
        val = iic_raw_bit();
	printf ( "SDA = %d\n", val );
	os_delay_us ( 1 );
    }
}

/* ARM */
static void
iic_clk_d ( int clk, int delay )
{
    iic_setclk ( clk );
    iic_watch ( delay );
}

/* ARM */
static int
iic_readbx ( void )
{
    int rv = 0;
    int i;
    int val;

    // GPIO_INPUT ( sda_pin );
    GPIO_INPUT ( sda_gpio, sda_pin );

    os_delay_us (5);

    // iic_dc ( cur_sda, 0 );
    iic_clk_d ( 0, 5 );

    for (i = 0; i < 8; i++) {
        // os_delay_us (5);
	iic_watch ( 5 );
	// iic_dc ( 1, 0 );
	iic_clk_d ( 0, 5 );
	// iic_dc ( 1, 1 );
	iic_clk_d ( 1, 5 );

        // rv |= GPIO_READ( sda_pin ) << (7-i);
        val = iic_raw_bit();
	rv |= val << (7-i);
	printf ( "*SDA = %d\n", val );

	// iic_dc_wait ( 1, 1, i == 7 ? 8 : 5 );
	iic_clk_d ( 1, i == 7 ? 8 : 5 );
    }

    // GPIO_OUTPUT ( sda_pin );
    GPIO_OUTPUT ( sda_gpio, sda_pin );

    iic_dc ( 1, 0 );

    return rv;
}
#endif

#ifdef ARCH_ESP8266
static int ICACHE_FLASH_ATTR
iic_readb ( void )
{
    int rv = 0;
    int i;

    os_delay_us (5);

    iic_dc ( cur_sda, 0 );

    for (i = 0; i < 8; i++) {
        os_delay_us (5);
	iic_dc ( 1, 0 );
	iic_dc ( 1, 1 );

        rv |= iic_get_bit () << (7-i);

	iic_dc_wait ( 1, 1, i == 7 ? 8 : 5 );
    }

    iic_dc ( 1, 0 );

    return rv;
}
#endif

static void ICACHE_FLASH_ATTR
iic_writeb ( int data )
{
    int bit;
    int i;

    os_delay_us (5);

    iic_dc ( cur_sda, 0 );

    for (i = 7; i >= 0; i--) {
        // bit = data >> i;
        bit = (data >> i) & 1;
	iic_dc ( bit, 0 );
	iic_dc_wait ( bit, 1, i == 0 ? 8 : 5 );
	iic_dc ( bit, 0 );
    }
}

static int ICACHE_FLASH_ATTR
iic_send_byte ( int byte )
{
	int ack;

	iic_writeb ( byte );
	ack = iic_getAck();
	if ( ack ) {
	    iic_stop();
	    return 1;
	}
	return 0;
}

/* XXX - useful when debugging
 *  but not what I would want when in production.
 */
static int ICACHE_FLASH_ATTR
iic_send_byte_m ( int byte, char *msg )
{
	int ack;

	iic_writeb ( byte );
	ack = iic_getAck();
	if ( ack ) {
	    printf("IIC: No ack after sending %s\n", msg);
	    iic_stop();
	    return 1;
	}
	return 0;
}

static int ICACHE_FLASH_ATTR
iic_recv_byte ( int ack )
{
	int rv;

	rv = iic_readb();
	iic_setAck ( ack );
	return rv;
}

/* ----------------------------------------------------------- */
/* Higher level iic routines */
/* ----------------------------------------------------------- */

#define IIC_WADDR(a)	(a << 1)
#define IIC_RADDR(a)	((a << 1) | 1)

/* raw write an array of bytes (8 bit objects)
 * for a device without registers (like the MCP4725)
 */
int ICACHE_FLASH_ATTR
iic_send ( int addr, unsigned char *buf, int n )
{
	int i;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	for ( i = 0; i < n; i++ ) {
		if ( iic_send_byte_m ( buf[i], "reg" ) ) return 1;
	}
	iic_stop();

	return 0;
}

/* raw read an array of bytes (8 bit objects)
 * for a device without registers (like the MCP4725)
 */
int ICACHE_FLASH_ATTR
iic_recv ( int addr, unsigned char *buf, int n )
{
	int i;

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return 1;

	for ( i=0; i < n; i++ ) {
		*buf++ = iic_recv_byte ( i == n - 1 ? 1 : 0 );
	}

	iic_stop();

	return 0;
}

#ifdef OLD_HIGH_LEVEL
/* raw read an array of shorts (16 bit objects)
 */
int ICACHE_FLASH_ATTR
iic_read_16raw ( int addr, unsigned short *buf, int n )
{
	unsigned int val;
	int i;

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return 1;

	for ( i=0; i < n; i++ ) {
		val =  iic_recv_byte ( 0 ) << 8;
		val |= iic_recv_byte ( i == n - 1 ? 1 : 0 );
		*buf++ = val;
	}

	iic_stop();

	return 0;
}

/* 8 bit read.
 * Send the address and see if we get an ACK.
 * ripped out of iic_read();
 */
void
iic_diag ( int addr, int reg )
{
	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) {
	    printf ( " Oops (addr) !!\n" );
	    return;
	}
	if ( iic_send_byte_m ( reg, "reg" ) ) {
	    printf ( " Oops (reg) !!\n" );
	    return;
	}
	iic_stop();
	printf ( " OK !!\n" );
}

/* 8 bit read */
int ICACHE_FLASH_ATTR
iic_read ( int addr, int reg )
{
	int rv;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return -1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return -1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return -1;
	rv = iic_recv_byte ( 1 );
	iic_stop();

	return rv;
}

/* 8 bit read */
int ICACHE_FLASH_ATTR
iic_readx ( int addr, int reg )
{
	int rv;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return -1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return -1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return -1;

	// rv = iic_recv_byte ( 1 );
	rv = iic_readbx();
	iic_setAck ( 1 );

	iic_stop();

	return rv;
}

/* read a 2 byte (short) object from
 * two consecutive i2c registers.
 * (or in some devices, a single 16 bit register)
 */
int ICACHE_FLASH_ATTR
iic_read_16 ( int addr, int reg )
{
	int rv;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return -1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return -1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return -1;
	rv =  iic_recv_byte ( 0 ) << 8;
	rv |= iic_recv_byte ( 1 );
	iic_stop();

	return rv;
}


/* read an array of bytes (8 bit objects) from
 * consecutive i2c registers.
 */
int ICACHE_FLASH_ATTR
iic_read_n ( int addr, int reg, unsigned char *buf, int n )
{
	unsigned int val;
	int i;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return 1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return 1;
	for ( i=0; i < n; i++ ) {
		*buf++ = iic_recv_byte ( i == n - 1 ? 1 : 0 );
	}
	iic_stop();

	return 0;
}

/* read an array of 2 byte (short) objects from
 * consecutive i2c registers.
 *  - note that i2c devices just read out consecutive registers
 *  until you send a nack
 */
int ICACHE_FLASH_ATTR
iic_read_16n ( int addr, int reg, unsigned short *buf, int n )
{
	int val;
	int i;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return 1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return 1;
	for ( i=0; i < n; i++ ) {
		val =  iic_recv_byte ( 0 ) << 8;
		val |= iic_recv_byte ( i == n - 1 ? 1 : 0 );
		*buf++ = val;
	}
	iic_stop();

	return 0;
}

/* read a 2 byte (short) object from
 * two consecutive i2c registers.
 */
int ICACHE_FLASH_ATTR
iic_read_24 ( int addr, int reg )
{
	int rv;

	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return -1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return -1;
	iic_stop();

	iic_start();
	if ( iic_send_byte_m ( IIC_RADDR(addr), "R address" ) ) return -1;
	rv =  iic_recv_byte ( 0 ) << 16;
	rv |=  iic_recv_byte ( 0 ) << 8;
	rv |= iic_recv_byte ( 1 );
	iic_stop();

	return rv;
}

int ICACHE_FLASH_ATTR
iic_write ( int addr, int reg, int val )
{
	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return 1;
	if ( iic_send_byte_m ( val, "val" ) ) return 1;
	iic_stop();

	return 0;
}

int ICACHE_FLASH_ATTR
iic_write16 ( int addr, int reg, int val )
{
	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return 1;
	if ( iic_send_byte_m ( val >> 8, "val_h" ) ) return 1;
	if ( iic_send_byte_m ( val & 0xff, "val_l" ) ) return 1;
	iic_stop();

	return 0;
}

/* write a register address with no data
 * (this could be a call to write_raw with a
 *   count of zero)
 */
int ICACHE_FLASH_ATTR
iic_write_nada ( int addr, int reg )
{
	iic_start();
	if ( iic_send_byte_m ( IIC_WADDR(addr), "W address" ) ) return 1;
	if ( iic_send_byte_m ( reg, "reg" ) ) return 1;
	iic_stop();

	return 0;
}
#endif /* OLD_HIGH_LEVEL */

/* THE END */
