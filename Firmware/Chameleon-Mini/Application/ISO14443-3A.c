/*
 * ISO14443A.c
 *
 *  Created on: 19.03.2013
 *      Author: skuser
 */

#include "ISO14443-3A.h"

#define CRC_INIT        0x6363
#define CRC_INIT_R      0xC6C6 /* Bit reversed */

#define USE_HW_CRC

#ifdef USE_HW_CRC
void ISO14443AAppendCRCA(void* Buffer, uint16_t ByteCount)
{
    uint8_t* DataPtr = (uint8_t*) Buffer;

    CRC.CTRL = CRC_RESET0_bm;
    CRC.CHECKSUM1 = (CRC_INIT_R >> 8) & 0xFF;
    CRC.CHECKSUM0 = (CRC_INIT_R >> 0) & 0xFF;
    CRC.CTRL = CRC_SOURCE_IO_gc;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Byte = BitReverseByte(Byte);

        CRC.DATAIN = Byte;
    }

    DataPtr[0] = BitReverseByte(CRC.CHECKSUM1);
    DataPtr[1] = BitReverseByte(CRC.CHECKSUM0);

    CRC.CTRL = CRC_SOURCE_DISABLE_gc;
}
#else
#include <util/crc16.h>
void ISO14443AAppendCRCA(void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = CRC_INIT;
    uint8_t* DataPtr = (uint8_t*) Buffer;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Checksum = _crc_ccitt_update(Checksum, Byte);
    }

    DataPtr[0] = (Checksum >> 0) & 0x00FF;
    DataPtr[1] = (Checksum >> 8) & 0x00FF;
}
#endif

#ifdef USE_HW_CRC
bool ISO14443ACheckCRCA(const void* Buffer, uint16_t ByteCount)
{
    const uint8_t* DataPtr = (const uint8_t*) Buffer;

    CRC.CTRL = CRC_RESET0_bm;
    CRC.CHECKSUM1 = (CRC_INIT_R >> 8) & 0xFF;
    CRC.CHECKSUM0 = (CRC_INIT_R >> 0) & 0xFF;
    CRC.CTRL = CRC_SOURCE_IO_gc;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Byte = BitReverseByte(Byte);

        CRC.DATAIN = Byte;
    }

    bool Result = (DataPtr[0] == BitReverseByte(CRC.CHECKSUM1)) && (DataPtr[1] == BitReverseByte(CRC.CHECKSUM0));

    CRC.CTRL = CRC_SOURCE_DISABLE_gc;

    return Result;
}
#else
#include <util/crc16.h>
bool ISO14443ACheckCRCA(const void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = CRC_INIT;
    const uint8_t* DataPtr = (const uint8_t*) Buffer;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;

        Checksum = _crc_ccitt_update(Checksum, Byte);
    }

    return (DataPtr[0] == ((Checksum >> 0) & 0xFF)) && (DataPtr[1] == ((Checksum >> 8) & 0xFF));
}
#endif

/* Coded in H to allow exportable inlining
INLINE bool ISO14443ASelect(void* Buffer, uint16_t* BitCount, uint8_t* UidCL, uint8_t SAKValue);
INLINE bool ISO14443AWakeUp(void* Buffer, uint16_t* BitCount, uint16_t ATQAValue, bool FromHalt);
*/
