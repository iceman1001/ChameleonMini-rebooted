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
#define MEMORY_ALL_MEMORY                   0xFFFFFFFF
#define MEMORY_NO_ADDR                      0
#ifdef CONFIG_MF_CLASSIC_SUPPORT
#define MEMORY_MIN_BYTES_PER_APP            MFCLASSIC_4K_MEM_SIZE
#else
#define MEMORY_MIN_BYTES_PER_APP            512
#endif
#define MEMORY_MAX_BYTES_PER_CARD_DIVIDER   8
#define MEMORY_MIN_BYTES_PER_SETTING        (sizeof(SettingsEntryType)+sizeof(SettingsType))

typedef struct {
    uint32_t maxFlashBytesPerSlot;
    uint32_t maxFlashBytesPerCardMemory;
    uint16_t maxEEPROMBytesPerSlot;
    bool isMemoryInit;
} memoryMappingInfo_t;

extern memoryMappingInfo_t MemoryMappingInfo;

bool MemoryInit(void);

/*
*
* Application/Config/Slot memory is divided in 2 separate spaces:
* - CardMemory to store card emulation data,
* - WorkingMemory to store internal application's data (logs, results, states, etc.).
*
* By default all operations are relative to current active setting's application memory
* space.
* Functions that end with "ForSetting" can be used to operate on a specified setting's
* application memory space, regardless of the active setting.
*
* We equally divides total SPI Flash memory space for each slot's application.
* CardMemory max amount is limited by MemoryMappingInfo.maxFlashBytesPerCardMemory.
* WorkingMemory is limited by all available slot's memory
* (MemoryMappingInfo.maxFlashBytesPerSlot) minus CardMemory.
* Memory spaces sizes are set in configuration definition.
* When MEMORY_ALL_MEMORY memory is set for a memory space, or if required memory is more
* than possible, then memory space will be given the maximum memory amount.
* As so, any memory operation should be based on this module memory spaces sizes results,
* and not on configuration constants for memory sizes.
*/

uint32_t AppCardMemorySizeForSetting(uint8_t SettingNumber);
uint32_t AppWorkingMemorySizeForSetting(uint8_t SettingNumber);
uint32_t AppMemorySizeForSetting(uint8_t SettingNumber);
uint32_t AppCardMemorySize(void);
uint32_t AppWorkingMemorySize(void);
uint32_t AppMemorySize(void);

bool AppCardMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppCardMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppCardMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);

bool AppCardMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppCardMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppCardMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount);
bool AppWorkingMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount);

bool MemoryClearAll(void);
bool AppMemoryClearForSetting(uint8_t SettingNumber);
bool AppMemoryClear(void);
bool AppCardMemoryClearForSetting(uint8_t SettingNumber);
bool AppCardMemoryClear(void);
bool AppWorkingMemoryClearForSetting(uint8_t SettingNumber);
bool AppWorkingMemoryClear(void);

#endif /* _MEM_MEMORY_H_ */
