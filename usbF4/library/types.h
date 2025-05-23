/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#ifndef _LIBMAPLE_TYPES_H_
#define _LIBMAPLE_TYPES_H_

#include <stdint.h>
#include <inttypes.h>

//typedef unsigned int	uint32_t;
//typedef unsigned short	uint16_t;
//typedef unsigned char	uint8_t;

typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

typedef volatile unsigned int	vu32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef void (*voidFuncPtr)(void);

#define __IO volatile

#ifndef __attr_flash
  #define __attr_flash __attribute__((section (".USER_FLASH")))
#endif

#ifndef NO_CCMRAM
#ifndef __attr_ccmram
  #define __attr_ccmram __attribute__((section (".ccmdata")))
#endif
#endif

#ifdef __always_inline
  #undef  __always_inline
#endif

#define __always_inline inline __attribute__((always_inline))

#ifndef NULL
  #define NULL 0
#endif

// Variable attributes, instructs the linker to place the marked
// variable in FLASH or CCRAM instead of RAM.
#define __FLASH__ __attr_flash

#ifndef NO_CCMRAM
#define __CCMRAM__ __attr_ccmram
#else
#define __CCMRAM__
#endif

#endif

/* THE END */
