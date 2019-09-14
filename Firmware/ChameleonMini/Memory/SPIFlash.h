/*
 * SPIFlash.h
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 * Parts are Copyright (c) 2014 Darran Hunt (darran [at] hunt dot net dot nz)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MEM_SPIFLASH_H_
#define _MEM_SPIFLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include "../Common.h"


#define FLASH_USART                 USARTD0
#define FLASH_PORT                  PORTD
#define FLASH_CS                    PIN4_bm
#define FLASH_MOSI                  PIN3_bm
#define FLASH_MISO                  PIN2_bm
#define FLASH_SCK                   PIN1_bm

#define FLASH_MDID_SIZE             5 // Bytes
#define FLASH_MDID_MID_OFFSET       0
#define FLASH_MDID_DID1_OFFSET      1
#define FLASH_MDID_DID2_OFFSET      2
#define FLASH_MDID_EDI_OFFSET       4
#define FLASH_DENSITY_FIRST         4 // Support starting at densityCode=4 flash memories
#define FLASH_DENSITY_LAST          8
#define FLASH_MDID_ID_OFFSET        FLASH_DENSITY_FIRST
#define FLASH_FAMILY_MASK           0xE0 // Bitmask for family field in flash device ID
#define FLASH_FAMILY_BITROLL        5
#define FLASH_FAMILY_CODE           0x01 // Supported flash family
#define FLASH_MANUFACTURER_ID       0x1F // Supported flash manufacturer
#define FLASH_DENSITY_MASK          0x1F // Bitmask for the flash density in the device ID
#define FLASH_SUBCODE_MASK          0xE0
#define FLASH_SUBCODE_BITROLL       5
#define FLASH_PRODUCT_MASK          0x1F

#define FLASH_OP_READ               0x0B // Random access continuous read (max freq)
                                         // Equivalent "1B" did not work during tests,
                                         // and has not difference for frequently found
                                         // ChameleonMini memories. Faster than "03".
#define FLASH_OP_READ_LF            0x03 // Random access continuous read (low freq)
#define FLASH_OP_PAGE_TO_BUF1       0x53 // Load a page to buffer 1
#define FLASH_OP_BUF1_WRITE         0x84 // Write into buffer 1
#define FLASH_OP_BUF1_TO_PAGE       0x83 // Dump buffer 1 into page
#define FLASH_OP_RWM_WITH_BUF1      0x58 // Read-Modify-Write through buffer 1
                                         // Note thats tests with Read-Modify-Write atomic
                                         // operations (58 and 59) failed.
#define FLASH_OP_GET_STATUS         0xD7 // Read status
#define FLASH_OP_SECTOR_ERASE       0x7C // Erase a sector
#define FLASH_OP_BLOCK_ERASE        0x50 // Erase a block
#define FLASH_OP_PAGE_ERASE         0x81 // Erase one page
#define	FLASH_OP_READ_DEV_ID        0x9F // Read Manufacturing and Device ID

#define FLASH_STATUS_BUSY           0x80 // Flash status busy bit
#define FLASH_STATUS_PAGESIZE_BIT   0x01 // Flash page size setting (0 is default, 1 is binary)

#define FLASH_DUMMY_BYTE            0x00
#define FLASH_CLEAR_BYTE            0xFF
#define FLASH_NO_OFFSET             0
#define FLASH_NO_PAGE               0
#define FLASH_B_IN_KB               1024
#define FLASH_BSIZE                 8 // Bits

#define FLASH_SEQ_PAGE_SIZE_BINARY  0x3D, 0x2A, 0x80, 0xA6 // Binary page size (256 bytes)
#define FLASH_SEQ_PAGE_SIZE_DEFAULT 0x3D, 0x2A, 0x80, 0xA7 // DataFlash (default) page size (264 bytes)
#define FLASH_SEQ_CHIP_ERASE        0xC7, 0x94, 0x80, 0x9A // erase entire chip

#define FLASH_SECTOR_ADDR_0A        0x00
#define FLASH_SECTOR_ADDR_0B        0x01

// Flash geometry
typedef struct {
    uint8_t dummyBitsInPageAddr;
    uint8_t dummyBitsInBlockAddr;
    uint8_t dummyBitsInSector0Addr;
    uint8_t dummyBitsInSectorNAddr;
    uint16_t bytesPerPage;
    uint16_t pagesNumber;
    uint16_t bytesPerBlock;
    uint16_t blocksNumber;
    uint32_t bytesPerSectorN;
    uint16_t bytesPerSector0a;
    uint32_t bytesPerSector0b;
    uint8_t sectorsNumber;
    uint8_t sizeMbits;
    uint16_t sizeKbytes;
    uint32_t sizeBytes;
} flashGeometry_t;

// Flash manufacturer info
typedef struct {
    uint8_t manufacturerId;
    uint8_t deviceId1;
    uint8_t deviceId2;
    uint8_t edi;
    uint8_t familyCode;
    uint16_t densityCode;
    uint8_t subCode;
    uint16_t productVariant;
    flashGeometry_t geometry;
} FlashInfo_t;

FlashInfo_t FlashInfo;

bool FlashInit(void);
bool FlashUnbufferedBytesRead(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool FlashBufferedBytesWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool FlashClearAll(void);
bool FlashClearRange(uint32_t Address, uint32_t ByteCount);

#endif /* _MEM_SPIFLASH_H_ */
