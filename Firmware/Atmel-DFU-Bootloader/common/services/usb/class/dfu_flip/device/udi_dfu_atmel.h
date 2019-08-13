/**
 * \file
 *
 * \brief USB Device Atmel Firmware Upgrade (Atmel DFU) interface definitions.
 *
 * Copyright (c) 2011-2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _UDI_DFU_ATMEL_H_
#define _UDI_DFU_ATMEL_H_

#include "conf_usb.h"
#include "usb_protocol.h"
#include "usb_protocol_dfu.h"
#include "udd.h"
#include "udc_desc.h"
#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup udi_dfu_atmel_group_udc
 * @{
 */
//! Global structure which contains standard UDI API for UDC
extern UDC_DESC_STORAGE udi_api_t udi_api_dfu_atmel;
//@}

/**
 * \ingroup udi_dfu_atmel_group
 * \defgroup udi_dfu_atmel_group_desc USB interface descriptors
 *
 * The following structures provide predefined USB interface descriptors.
 * It must be used to define the final USB descriptors.
 */
//@{

//! Interface descriptor structure for Atmel DFU
typedef struct {
	usb_iface_desc_t iface;
} udi_dfu_atmel_desc_t;

//! By default no string associated to this interface
#ifndef UDI_DFU_ATMEL_STRING_ID
#define UDI_DFU_ATMEL_STRING_ID     0
#endif

//! Content of DFU interface descriptor for Full Speed
#define UDI_DFU_ATMEL_DESC      {\
	.iface.bLength             = sizeof(usb_iface_desc_t),\
	.iface.bDescriptorType     = USB_DT_INTERFACE,\
	.iface.bInterfaceNumber    = UDI_DFU_ATMEL_IFACE_NUMBER,\
	.iface.bAlternateSetting   = 0,\
	.iface.bNumEndpoints       = 0,\
	.iface.bInterfaceClass     = CLASS_VENDOR_SPECIFIC,\
	.iface.bInterfaceSubClass  = 0,\
	.iface.bInterfaceProtocol  = 0,\
	.iface.iInterface          = UDI_DFU_ATMEL_STRING_ID,\
	}
//@}


/**
 * \ingroup udi_group
 * \defgroup udi_dfu_atmel_group USB Device Interface (UDI) for Device Firmware Upgrade Atmel specific
 *
 * The DFU from Atmel is based on DFU specification,
 * but does not implement same protocol.
 *
 * The USB Device Atmel DFU class implement both version:
 * - The <A href="http://www.atmel.com/Images/doc7618.pdf">
 * doc7618: USB DFU Bootloader Datasheet</A> describes the
 * FLIP USB DFU Protocol version 1 used by Mega devices.
 * - The <A href="http://www.atmel.com/Images/doc32131.pdf">
 * AVR32760: AVR32 UC3 USB DFU Bootloader Protocol</A>  and the
 * <A href="http://www.atmel.com/Images/doc8457.pdf">
 * AVR4023: FLIP USB DFU Protocol</A> describes the
 * FLIP USB DFU Protocol version 2 used by UC3 and Xmega devices.
 *
 * The interface between the DFU Atmel Class and device is done through the 
 * \ref isp service.
 * @{
 */
//@}

#ifdef __cplusplus
}
#endif
#endif // _UDI_DFU_ATMEL_H_
