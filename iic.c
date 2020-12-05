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
 * 12-4-2020 - moved to Hydra with big cleanup.
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
 *    void iic_init ( sda_gpio, sda_pin, sclk_gpio, sclk_pin );
 *    (previously -- ) void iic_init ( sda_pin, sclk_pin );
 *    int iic_send ( addr, unsigned char *, int );
 *    int iic_recv ( addr, unsigned char *, int );
 */

typedef unsigned char uint8;

/* For libmaple-unwired */
// #define ARCH_MAPLE

/* For Kyu */
// #define ARCH_KYU

/* For Hydra */
#define ARCH_HYDRA

#ifdef ARCH_HYDRA
#define GPIO_INPUT(g,p)		gpio_input_config ( g, p )
#define GPIO_OUTPUT(g,p)	gpio_output_od_config ( g, p )
#define GPIO_READ(g,p)		gpio_read ( g, p )

// #define GPIO_SET(g,p)	gpio_bit ( g, p, 1 )
// #define GPIO_CLEAR(g,p)	gpio_bit ( g, p, 0 )
#define GPIO_VAL(g,p,x)	gpio_bit ( g, p, x )
#endif

/* On the Maple/STM32, we have a handy upper layer on the more fundamental
 * lower gpio layer.  The lower layer requires a gpio device and pin.
 * The upper layer requires a single index that indexes pinmap transparently
 * hiding the details of device/pin.  So we use the upper layer.
 * (we abandon that in Hydra though)
 */

#ifdef ARCH_MAPLE
#include "serial.h"
#include "time.h"
#include "gpio.h"
#include "io.h"

#define GPIO_INPUT(x)	pinMode ( x, INPUT_FLOATING )
#define GPIO_OUTPUT(x)	pinMode ( x, OUTPUT_OPEN_DRAIN )
#define GPIO_READ(x)	digitalRead ( x )

#define GPIO_SET(x)	digitalWrite ( x, 1 )
#define GPIO_CLEAR(x)	digitalWrite ( x, 0 )
#endif

#ifdef ARCH_KYU
#include "gpio.h"

#define GPIO_INPUT(x)	gpio_dir_in ( x )
#define GPIO_OUTPUT(x)	gpio_dir_out ( x )
#define GPIO_READ(x)	gpio_read_bit ( x )

#define GPIO_SET(x)	gpio_set_bit ( x )
#define GPIO_CLEAR(x)	gpio_clear_bit ( x )
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

/* These do not require 0/1 return values from GPIO_READ */
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

static void
iic_dc ( int data, int clock )
{
    iic_setdc ( data, clock );
    delay_us ( 5 );
}

static void
iic_dc_wait ( int data, int clock, int wait )
{
    iic_setdc ( data, clock );
    delay_us ( wait );
}

static void
iic_start(void)
{
    iic_dc ( 1, cur_scl );
    iic_dc ( 1, 1 );
    iic_dc ( 0, 1 );
}

static void
iic_stop(void)
{
    delay_us (5);

    iic_dc ( 0, cur_scl );
    iic_dc ( 0, 1 );
    iic_dc ( 1, 1 );
}

static void
iic_setAck ( int level )
{
    iic_dc ( cur_sda, 0 );
    iic_dc ( level, 0 );
    iic_dc ( level, 1 );
    iic_dc ( level, 0 );
    iic_dc ( 1, 0 );
}

static int
iic_getAck ( void )
{
    int rv;

    iic_dc ( cur_sda, 0 );
    iic_dc ( 1, 0 );
    iic_dc ( 1, 1 );

    rv = iic_get_bit ();
    delay_us (5);

    iic_dc ( 1, 0 );

    return rv;
}

/* XXX - why even manipulate the sda pin when reading ?? */
/* We send the Ack outside of this routine */
static int
iic_readb ( void )
{
    int rv = 0;
    int i;

    // GPIO_INPUT ( sda_pin );
    GPIO_INPUT ( sda_gpio, sda_pin );

    delay_us (5);

    iic_dc ( cur_sda, 0 );

    for (i = 0; i < 8; i++) {
        delay_us (5);
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
	delay_us ( 1 );
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

    delay_us (5);

    // iic_dc ( cur_sda, 0 );
    iic_clk_d ( 0, 5 );

    for (i = 0; i < 8; i++) {
        // delay_us (5);
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

static void
iic_writeb ( int data )
{
    int bit;
    int i;

    delay_us (5);

    iic_dc ( cur_sda, 0 );

    for (i = 7; i >= 0; i--) {
        // bit = data >> i;
        bit = (data >> i) & 1;
	iic_dc ( bit, 0 );
	iic_dc_wait ( bit, 1, i == 0 ? 8 : 5 );
	iic_dc ( bit, 0 );
    }
}

static int
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
static int
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

static int
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
int
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
int
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

/* THE END */
