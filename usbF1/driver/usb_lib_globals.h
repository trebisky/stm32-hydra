/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#ifndef _USB_LIB_GLOBALS_H_
#define _USB_LIB_GLOBALS_H_

/* usb_lib headers */
#ifdef HYDRA
#include "../lib/usb_type.h"
#include "../lib/usb_core.h"
#else
#include "usb_type.h"
#include "usb_core.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern USER_STANDARD_REQUESTS  User_Standard_Requests;
extern USER_STANDARD_REQUESTS *pUser_Standard_Requests;

extern DEVICE_PROP  Device_Property;
extern DEVICE_PROP *pProperty;

extern DEVICE_INFO  Device_Info;
extern DEVICE_INFO *pInformation;

extern DEVICE Device_Table;
extern u16 SaveRState;
extern u16 SaveTState;

#ifdef __cplusplus
}
#endif

#endif
