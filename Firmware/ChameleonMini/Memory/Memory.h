/*
 * Memory.h
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#ifndef _MEM_MEMORY_H_
#define _MEM_MEMORY_H_

#include "../Settings.h" // Just to define constants
#include "../Configuration.h" // Just to define constants
#include "../Application/Application.h" // Just to define constants
#include "../Application/MifareClassic.h" // Just to define constants

#define MEMORY_NO_MEMORY                    0x00
#define MEMORY_NO_ADDR                      0
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
#define MEMORY_MIN_BYTES_PER_APP            MIFARE_CLASSIC_4K_MEM_SIZE
#else
#define MEMORY_MIN_BYTES_PER_APP            MIFARE_CLASSIC_1K_MEM_SIZE
#endif
#define MEMORY_MIN_BYTES_PER_SETTING        (sizeof(SettingsEntryType)+sizeof(SettingsType))

typedef struct {
    uint32_t maxFlashBytesPerSlot;
    uint16_t maxEEPROMBytesPerSlot;

} memoryMappingInfo_t;

memoryMappingInfo_t MemoryMappingInfo;

bool MemoryInit(void);

/*
* By default all operations are relative to current active setting's application memory
* space.
* Functions that end with "ForSetting" can be used to operate on a specified setting's
* application memory space, regardless of the active setting.
*
* We equally divides total SPI Flash memory space for each setting/slot's application
* dedicated memory space.
* But we will actually use setting's configuration required memory only, by default, except
* when full available space is required, either by requiring 0 memory in setting's
* configuration, or by requiring more than total available memory per setting/slot.
*/

bool AppMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);

bool AppMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);

bool MemoryClearAll(void);
bool AppMemoryClearForSetting(uint8_t SettingNumber);
bool AppMemoryClear(void);

#endif /* _MEM_MEMORY_H_ */
