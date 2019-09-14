/*
 * SPIFlash.c
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

#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "SPIFlash.h"
#include "../Common.h"

// Operating parameters for the different size flash chips that are supported
static const flashGeometry_t AT45DBXX1E[] PROGMEM = {
/*
ID = densityCode - FLASH_MDID_ID_OFFSET
Sector (s) > Block (b) > Page (p) > Byte (B)
53,50,7c,7c,B/p,   np, B/b,  nb,  B/sN, /0a,   /0b,ns,sM, sKB,sB
*/
// 4MB ID:00100=4 AT45DB041E
{8,11,11,16,256, 2048,2048, 256, 65536,2048, 63488, 8, 4, 512, 524288},
// 8MB ID:00101=5 AT45DB081E
{8,11,11,16,256, 4096,2048, 512, 65536,2048, 63488,16, 8,1024,1048576},
// 16MB ID:00110=6 AT45DB161E
{9,12,12,17,512, 4096,4096, 512,131072,4096,126976,16,16,2048,2097152},
// 32MB ID:00111=7 AT45DB321E
{9,12,12,16,512, 8192,4096,1024, 65536,4096, 61440,64,32,4096,4194304},
// 64MB ID:01000=8 AT45DB641E
{9, 3, 3,10,256,32768,2048,4096,262144,2048,260096,32,64,8192,8388608}
};

// Tells if Flash was correctly initialized
static bool isFlashInit = false;

/* Common helpers for SPI FLash commands
***************************************************************************************/

INLINE void OPStart(void) {
    FLASH_PORT.OUTCLR = FLASH_CS;
}

INLINE void OPStop(void) {
    FLASH_PORT.OUTSET = FLASH_CS;
}

INLINE uint8_t SPITransferByte(uint8_t Data)
{
    FLASH_USART.DATA = Data;
    while (!(FLASH_USART.STATUS & USART_TXCIF_bm));
    while (!(FLASH_USART.STATUS & USART_RXCIF_bm));
    return FLASH_USART.DATA;
}

INLINE void SPIReadBlock(void* Buffer, uint16_t ByteCount)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    while(ByteCount--) {
        *ByteBuffer++ = SPITransferByte(FLASH_DUMMY_BYTE);
    }
}

INLINE void SPIWriteBlock(const void* Buffer, uint16_t ByteCount)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    while(ByteCount--) {
        SPITransferByte(*ByteBuffer++);
    }
}

INLINE uint8_t FlashReadStatusRegister(void)
{
    uint8_t Register;
    OPStart();
    SPITransferByte(FLASH_OP_GET_STATUS);
    Register = SPITransferByte(FLASH_DUMMY_BYTE);
    OPStop();
    return Register;
}

INLINE void WaitForReadyFlash(void) {
    while(!(FlashReadStatusRegister() & FLASH_STATUS_BUSY));
}

INLINE bool checkAddrConsistency(uint32_t Address, uint32_t ByteCount) {
    return ( isFlashInit
             && (ByteCount > FLASH_NO_OFFSET)
             && (Address <= (FlashInfo.geometry.sizeBytes - ByteCount)) );
}

INLINE void sendAddrOp(uint8_t Op, uint32_t Address) {
    uint8_t opseq[] = { Op, ((uint8_t *)&Address)[2], ((uint8_t *)&Address)[1], ((uint8_t *)&Address)[0] };
    SPIWriteBlock(opseq, sizeof(opseq));
}

/* Memory init and information gathering operations
***************************************************************************************/

// Get sure we set the binary page format
// During tests, ChameleonMini SPI flash chip could not be set to DataFlash
// format, probably because of faulty chips, or previous configuration overflow
// (page size configuration is limited to 10000 configurations).
INLINE void FlashConfigurePageSize(void) {
    if( !(FlashReadStatusRegister() & FLASH_STATUS_PAGESIZE_BIT) ) {
        uint8_t opseq[] = { FLASH_SEQ_PAGE_SIZE_BINARY };
        WaitForReadyFlash();
        OPStart();
        SPIWriteBlock(opseq, sizeof(opseq));
        OPStop();
        WaitForReadyFlash();
    }
}

INLINE bool FillFlashInfo(void) {
    bool ret = false;
    uint8_t Data[FLASH_MDID_SIZE];
    WaitForReadyFlash();
    OPStart();
    SPITransferByte(FLASH_OP_READ_DEV_ID);
    SPIReadBlock(Data, FLASH_MDID_SIZE);
    OPStop();
    FlashInfo.manufacturerId = Data[FLASH_MDID_MID_OFFSET];
    FlashInfo.deviceId1 = Data[FLASH_MDID_DID1_OFFSET];
    FlashInfo.deviceId2 = Data[FLASH_MDID_DID2_OFFSET];
    FlashInfo.edi = Data[FLASH_MDID_EDI_OFFSET];
    FlashInfo.familyCode = (FlashInfo.deviceId1 & FLASH_FAMILY_MASK) >> FLASH_FAMILY_BITROLL;
    FlashInfo.densityCode = FlashInfo.deviceId1 & FLASH_DENSITY_MASK;
    FlashInfo.subCode = (FlashInfo.deviceId2 & FLASH_SUBCODE_MASK) >> FLASH_SUBCODE_BITROLL;
    FlashInfo.productVariant = FlashInfo.deviceId2 & FLASH_PRODUCT_MASK;
    if( (FlashInfo.densityCode >= FLASH_DENSITY_FIRST)
        && (FlashInfo.densityCode <= FLASH_DENSITY_LAST) ) {
        memcpy_P( &FlashInfo.geometry, &(AT45DBXX1E[FlashInfo.densityCode - FLASH_MDID_ID_OFFSET]), sizeof(FlashInfo.geometry) );
        ret = true;
    }
    return ret;
}

bool FlashInit(void) {
    bool ret = false;
    OPStop();
    // Configure FLASH_USART for SPI master mode 0 with max clock frequency
    FLASH_PORT.OUTCLR = FLASH_SCK;
    FLASH_PORT.OUTSET = FLASH_MOSI;
    FLASH_PORT.DIRSET = FLASH_SCK | FLASH_MOSI | FLASH_CS;
    FLASH_USART.BAUDCTRLA = FLASH_NO_OFFSET;
    FLASH_USART.BAUDCTRLB = FLASH_NO_OFFSET;
    FLASH_USART.CTRLC = USART_CMODE_MSPI_gc;
    FLASH_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
    if ( FillFlashInfo()
         && (FlashInfo.manufacturerId == FLASH_MANUFACTURER_ID)
         && (FlashInfo.familyCode == FLASH_FAMILY_CODE) ) {
        FlashConfigurePageSize();
        ret = true;
        isFlashInit = ret;
    }
    return ret;
}

/* Memory read operations
***************************************************************************************/

bool FlashUnbufferedBytesRead(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    if( checkAddrConsistency(Address, ByteCount) ) {
        WaitForReadyFlash();
        OPStart();
        sendAddrOp(FLASH_OP_READ, Address);
        // 1 dummy byte is required after command for full speed reading, by spec...
        SPITransferByte(FLASH_DUMMY_BYTE);
        SPIReadBlock(Buffer, ByteCount);
        OPStop();
        ret = true;
    }
    return ret;
}

/* Memory write operations
***************************************************************************************/

bool FlashBufferedBytesWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    if( checkAddrConsistency(Address, ByteCount) ) {
        uint32_t Head = FLASH_NO_OFFSET;
        uint32_t PageNum, Offset, ByteRoll;
        do {
            PageNum = ((uint32_t)(Address / FlashInfo.geometry.bytesPerPage)) << FlashInfo.geometry.dummyBitsInPageAddr;
            Offset = (Address % FlashInfo.geometry.bytesPerPage);
            ByteRoll = (ByteCount >= FlashInfo.geometry.bytesPerPage) ? (FlashInfo.geometry.bytesPerPage - Offset) : (ByteCount);
            WaitForReadyFlash();
            OPStart();
            sendAddrOp(FLASH_OP_PAGE_TO_BUF1, PageNum);
            OPStop();
            WaitForReadyFlash();
            OPStart();
            sendAddrOp(FLASH_OP_BUF1_WRITE, Offset);
            SPIWriteBlock(Buffer+Head, ByteRoll);
            OPStop();
            WaitForReadyFlash();
            OPStart();
            sendAddrOp(FLASH_OP_BUF1_TO_PAGE, PageNum);
            OPStop();
            ByteCount -= ByteRoll;
            Address += ByteRoll;
            Head += ByteRoll;
        } while(ByteCount);
        ret = true;
    }
    return ret;
}

/* Memory erase operations
***************************************************************************************/

bool FlashClearPage(uint16_t PageNum) {
    bool ret = false;
    if( isFlashInit && (PageNum < FlashInfo.geometry.pagesNumber) ) {
        WaitForReadyFlash();
        OPStart();
        sendAddrOp(FLASH_OP_PAGE_ERASE, ((uint32_t)PageNum) << FlashInfo.geometry.dummyBitsInPageAddr);
        OPStop();
        ret = true;
    }
    return ret;
}

bool FlashClearBlock(uint16_t BlockNum) {
    bool ret = false;
    if( isFlashInit && (BlockNum < FlashInfo.geometry.blocksNumber) ) {
        WaitForReadyFlash();
        OPStart();
        sendAddrOp(FLASH_OP_BLOCK_ERASE, BlockNum << FlashInfo.geometry.dummyBitsInBlockAddr);
        OPStop();
        ret = true;
    }
    return ret;
}

bool FlashClearSector(uint16_t SectorNum) {
    bool ret = false;
    if( isFlashInit && (SectorNum < FlashInfo.geometry.sectorsNumber) ) {
        bool retblock = true;
        uint32_t sector = FLASH_NO_OFFSET;
        WaitForReadyFlash();
        OPStart();
        if (SectorNum > FLASH_NO_OFFSET) {
            sector = ((uint32_t)SectorNum) << FlashInfo.geometry.dummyBitsInSectorNAddr;
        } else if (SectorNum == FLASH_NO_OFFSET) {
            // Sector 0a to be deleted by FlashClearBlock on Block 0
            FlashClearBlock(FLASH_NO_OFFSET);
            sector = ((uint32_t)FLASH_SECTOR_ADDR_0B) << FlashInfo.geometry.dummyBitsInSector0Addr;
        }
        sendAddrOp(FLASH_OP_SECTOR_ERASE, sector);
        OPStop();
        ret = retblock;
    }
    return ret;
}

bool FlashClearAll(void) {
    bool ret = false;
    if (isFlashInit) {
        uint8_t opseq[] = { FLASH_SEQ_CHIP_ERASE };
        WaitForReadyFlash();
        OPStart();
        SPIWriteBlock(opseq, sizeof(opseq));
        OPStop();
        // Clearing might be long, so wait for memory to be ready before returning
        WaitForReadyFlash();
        ret = true;
    }
    return ret;
}

INLINE void FlashClearRangeRound(uint32_t BytesPerItem, uint32_t * Address, uint32_t * ByteCount) {
    uint16_t nbitems = *ByteCount / BytesPerItem;
    // If range is larger than 1 Item
    if(nbitems) {
        uint16_t nbitemsBefore = (*Address) / BytesPerItem;
        uint16_t ItemNum = FLASH_NO_OFFSET;
        // If our start address match a item (sector, block, page) start, we start at it
        if( ((*Address) % BytesPerItem) == FLASH_NO_OFFSET ) {
            ItemNum = nbitemsBefore;
            (*Address) += (nbitems * BytesPerItem);
            (*ByteCount) %= BytesPerItem;
        // Else we start at next one, only if there are several items, to avoid
        // deleting an item that may overlap an address after Address+ByteCount
        } else if( nbitems > 1 ) {
            nbitems--;
            ItemNum = nbitemsBefore + 1;
            uint16_t headerOffset = (ItemNum * BytesPerItem) - (*Address);
            // Recurse for header offset
            if( headerOffset > FLASH_NO_OFFSET) {
                FlashClearRange((*Address), headerOffset);
            }
            (*Address) += (nbitems * BytesPerItem);
            (*ByteCount) -= (headerOffset + (nbitems * BytesPerItem));
        } else {
            nbitems = FLASH_NO_OFFSET;
        }
        bool (*FlashClearItem)(uint16_t);
        if( BytesPerItem == FlashInfo.geometry.bytesPerSectorN ) {
            FlashClearItem = &FlashClearSector;
        } else if ( BytesPerItem == FlashInfo.geometry.bytesPerBlock ) {
            FlashClearItem = &FlashClearBlock;
        } else {
            FlashClearItem = &FlashClearPage;
        }
        // Delete items we can
        while(nbitems--) {
            (*FlashClearItem)(ItemNum);
            ItemNum++;
        }
    }
}

/* Try to optimally erase a range of flash memory, by deleting sectors first, then blocks
* and finally pages */
bool FlashClearRange(uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    // We cannot clear less than a Page
    if( ByteCount >= FlashInfo.geometry.bytesPerPage ) {
        // If we want to clear all bytes from Address 0, then we FlashClearAll
        if( (Address == FLASH_NO_OFFSET) && (ByteCount == FlashInfo.geometry.sizeBytes) ) {
            ret = FlashClearAll();
        } else if( checkAddrConsistency(Address, ByteCount) ) {
            // Sectors clear round
            FlashClearRangeRound(FlashInfo.geometry.bytesPerSectorN, &Address, &ByteCount);
            // Blocks clear round
            FlashClearRangeRound(FlashInfo.geometry.bytesPerBlock, &Address, &ByteCount);
            // Pages clear round
            FlashClearRangeRound(FlashInfo.geometry.bytesPerPage, &Address, &ByteCount);
            ret = true;
        }
    }
    return ret;
}
