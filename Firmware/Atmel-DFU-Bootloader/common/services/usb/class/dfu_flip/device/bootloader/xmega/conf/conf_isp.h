/**
 * \file
 *
 * \brief ISP configuration file.
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

#ifndef _CONF_ISP_H_
#define _CONF_ISP_H_

#include "parts.h"

#if !defined(__ASSEMBLER__) && !defined(__IAR_SYSTEMS_ASM__)
// This define may only be included from C files

#include "compiler.h"
#include "nvm.h"


// Bootloader Versions
// Example: Version 0x00 give 1.0.0 on batchisp log
// Example: Version 0x03 give 1.0.3 on batchisp log
// Example: Version 0x25 give 1.2.5 on batchisp log
#  define BOOTLOADER_VERSION   0x04

#  if (FLASH_SIZE<=0x10000) // FLASH <= 64KB
// If all memories (flash,eeprom,...) do not exceed 64KB.
// then the ISP interface can be optimized to save CODE.
#    define ISP_SMALL_MEMORY_SIZE
#  endif

#endif

// Definition of hardware condition to enter in ISP mode
#if XMEGA_A1U
# define ISP_PORT_DIR      PORTF_DIR
# define ISP_PORT_PINCTRL  PORTF_PIN5CTRL
# define ISP_PORT_IN       PORTF_IN
# define ISP_PORT_PIN      0
#elif (XMEGA_A3U || XMEGA_A3BU || XMEGA_C3)
# define ISP_PORT_DIR      PORTE_DIR
# define ISP_PORT_PINCTRL  PORTE_PIN5CTRL
# define ISP_PORT_IN       PORTE_IN
# define ISP_PORT_PIN      5
#elif (XMEGA_A4U || XMEGA_C4)
// Changed default XMEGA_A4U to fit RevE rebooted.
//# define ISP_PORT_DIR      PORTC_DIR
//# define ISP_PORT_PINCTRL  PORTC_PIN3CTRL
//# define ISP_PORT_IN       PORTC_IN
//# define ISP_PORT_PIN      3
# define ISP_PORT_DIR      PORTA_DIR
# define ISP_PORT_PINCTRL  PORTA_PIN6CTRL
# define ISP_PORT_IN       PORTA_IN
# define ISP_PORT_PIN      6
#elif XMEGA_B
# define ISP_PORT_DIR      PORTC_DIR
# define ISP_PORT_PINCTRL  PORTC_PIN6CTRL
# define ISP_PORT_IN       PORTC_IN
# define ISP_PORT_PIN      6
#else
# error Unknow AVR Xmega part
#endif

#endif // _CONF_ISP_H_
