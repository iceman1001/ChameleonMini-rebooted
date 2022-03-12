/*
 * Memory.c
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#include <stdint.h>
#include <ctype.h>
#include <avr/io.h>
#include "EEPROM.h"
#include "SPIFlash.h"
#include "Memory.h"
#include "../Common.h"
#include "../Settings.h"
#include "../Configuration.h"
#include "../Application/Application.h"

memoryMappingInfo_t MemoryMappingInfo = {MEMORY_NO_MEMORY, MEMORY_NO_MEMORY, MEMORY_NO_MEMORY, false};

/* Common helpers for memory operations
***************************************************************************************/

// How much memory a setting's application will take. If set to MEMORY_ALL_MEMORY
// or to more than possible, use max available memory.
// Otherwise, give what is required by setting's configuration values.
uint32_t getAppSomeMemorySizeForSetting(uint32_t availMem, uint32_t requiredMem) {
    if( (requiredMem == MEMORY_ALL_MEMORY) || (requiredMem > availMem) ) {
        requiredMem = availMem;
    }
    return requiredMem;
}

uint32_t AppCardMemorySizeForSetting(uint8_t SettingNumber) {
    uint32_t requiredMem = (SettingNumber == GlobalSettings.ActiveSettingIdx) ? (ActiveConfiguration.CardMemorySize) : (ConfigurationTableGetCardMemorySizeForId(GlobalSettings.Settings[SettingNumber].Configuration));
    return getAppSomeMemorySizeForSetting( MemoryMappingInfo.maxFlashBytesPerCardMemory, requiredMem);
}

uint32_t AppWorkingMemorySizeForSetting(uint8_t SettingNumber) {
    uint32_t requiredMem = (SettingNumber == GlobalSettings.ActiveSettingIdx) ? (ActiveConfiguration.WorkingMemorySize) : (ConfigurationTableGetWorkingMemorySizeForId(GlobalSettings.Settings[SettingNumber].Configuration));
    return getAppSomeMemorySizeForSetting( (MemoryMappingInfo.maxFlashBytesPerSlot - AppCardMemorySizeForSetting(SettingNumber)), requiredMem );
}

uint32_t AppCardMemorySize(void) {
    return AppCardMemorySizeForSetting(GlobalSettings.ActiveSettingIdx);
}

uint32_t AppWorkingMemorySize(void) {
    return AppWorkingMemorySizeForSetting(GlobalSettings.ActiveSettingIdx);
}

uint32_t AppMemorySizeForSetting(uint8_t SettingNumber) {
    return (AppCardMemorySizeForSetting(SettingNumber) + AppWorkingMemorySizeForSetting(SettingNumber));
}

uint32_t AppMemorySize(void) {
    return AppMemorySizeForSetting(GlobalSettings.ActiveSettingIdx);
}

bool checkSettingNumberConsistency(uint8_t SettingNumber) {
    return ( (SettingNumber == GlobalSettings.ActiveSettingIdx) || ((SettingNumber >= SETTINGS_FIRST) && (SettingNumber <= SETTINGS_LAST)) );
}

// Is an address start + offset R/W operation valid in setting's application memory space
bool checkAddrConsistencyForSetting(uint32_t availMem, uint8_t SettingNumber, uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    if( MemoryMappingInfo.isMemoryInit && checkSettingNumberConsistency(SettingNumber)
        && (availMem > MEMORY_NO_MEMORY) && (availMem >= ByteCount)) {
        ret = (Address <= (availMem - ByteCount));
    }
    return ret;
}

bool checkCardMemAddrConsistencyForSetting(uint8_t SettingNumber, uint32_t Address, uint32_t ByteCount) {
    uint32_t availMem = (SettingNumber == GlobalSettings.ActiveSettingIdx) ? (AppCardMemorySize()) : (AppCardMemorySizeForSetting(SettingNumber));
    return checkAddrConsistencyForSetting(availMem, SettingNumber, Address, ByteCount);
}

bool checkWorkingMemAddrConsistencyForSetting(uint8_t SettingNumber, uint32_t Address, uint32_t ByteCount) {
    uint32_t availMem = (SettingNumber == GlobalSettings.ActiveSettingIdx) ? (AppWorkingMemorySize()) : (AppWorkingMemorySizeForSetting(SettingNumber));
    return checkAddrConsistencyForSetting(availMem, SettingNumber, Address, ByteCount);;
}

// Returns a byte address in SPI Flash from a byte address relative to application's
// memory space.
// Does not check for address validity in application's space (checkSettingAddrConsistency
// must be used if needed).
uint32_t getFlashAddressForSetting(uint8_t SettingNumber, uint32_t Address) {
    return ((uint32_t)(SettingNumber * MemoryMappingInfo.maxFlashBytesPerSlot) + Address);
}

uint32_t getCardMemFlashAddressForSetting(uint8_t SettingNumber, uint32_t Address) {
    return getFlashAddressForSetting(SettingNumber, Address);
}

uint32_t getWorkingMemFlashAddressForSetting(uint8_t SettingNumber, uint32_t Address) {
    return getFlashAddressForSetting(SettingNumber, (AppCardMemorySizeForSetting(SettingNumber) + Address));
}

/* Memory init operations
***************************************************************************************/

// Init memory mapping and return false if not enough memory or not supported chip.
bool MemoryInit(void) {
    bool flashOk = false;
    bool eepromOk = false;
    if( FlashInit() ) {
        MemoryMappingInfo.maxFlashBytesPerSlot = (FlashInfo.geometry.sizeBytes / SETTINGS_COUNT);
        MemoryMappingInfo.maxFlashBytesPerCardMemory = (MemoryMappingInfo.maxFlashBytesPerSlot / MEMORY_MAX_BYTES_PER_CARD_DIVIDER);
        if( MemoryMappingInfo.maxFlashBytesPerCardMemory >= MEMORY_MIN_BYTES_PER_APP ) {
            flashOk = true;
        } else {
            MemoryMappingInfo.maxFlashBytesPerSlot = MEMORY_NO_MEMORY;
            MemoryMappingInfo.maxFlashBytesPerCardMemory = MEMORY_NO_MEMORY;
        }
    }
    if( EEPROMInit() ) {
        MemoryMappingInfo.maxEEPROMBytesPerSlot = (EEPROMInfo.bytesTotal / SETTINGS_COUNT);
        if( MemoryMappingInfo.maxEEPROMBytesPerSlot >= MEMORY_MIN_BYTES_PER_SETTING ) {
            eepromOk = true;
        } else {
            MemoryMappingInfo.maxFlashBytesPerSlot = MEMORY_NO_MEMORY;
        }
    }
    MemoryMappingInfo.isMemoryInit = (flashOk && eepromOk);
    return MemoryMappingInfo.isMemoryInit;
}


/* Memory read operations
***************************************************************************************/

bool AppMemoryReadForSetting( bool (*checkAddr)(uint8_t, uint32_t, uint32_t),
                              uint32_t (*getAddr)(uint8_t, uint32_t),
                              uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount ) {
    bool ret = false;
    if( (*checkAddr)(SettingNumber, Address, ByteCount) ) {
        ret = FlashUnbufferedBytesRead(Buffer, (*getAddr)(SettingNumber, Address), ByteCount);
    }
    return ret;
}

bool AppCardMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryReadForSetting( &checkCardMemAddrConsistencyForSetting, &getCardMemFlashAddressForSetting,
                                    SettingNumber, Buffer, Address, ByteCount );
}

bool AppWorkingMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryReadForSetting( &checkWorkingMemAddrConsistencyForSetting, &getWorkingMemFlashAddressForSetting,
                                    SettingNumber, Buffer, Address, ByteCount );
}

bool AppCardMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppCardMemoryReadForSetting(GlobalSettings.ActiveSettingIdx, Buffer, Address, ByteCount);
}

bool AppWorkingMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppWorkingMemoryReadForSetting(GlobalSettings.ActiveSettingIdx, Buffer, Address, ByteCount);
}

bool AppMemoryDownloadXModem( uint32_t (*getSize)(void), bool (*memRead)(void*, uint32_t, uint32_t),
                              void* Buffer, uint32_t Address, uint32_t ByteCount ) {
    bool ret = false;
    uint32_t AvailBytes = (*getSize)();
    if(Address < AvailBytes) {
        uint32_t BytesLeft = MIN(ByteCount, AvailBytes - Address);
        ret = (*memRead)(Buffer, Address, BytesLeft);
    }
    return ret;
}

bool AppCardMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryDownloadXModem( &AppCardMemorySize, &AppCardMemoryRead, Buffer, Address, ByteCount);
}

bool AppWorkingMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryDownloadXModem( &AppWorkingMemorySize, &AppWorkingMemoryRead, Buffer, Address, ByteCount);
}

/* Memory write operations
***************************************************************************************/

bool AppMemoryWriteForSetting( bool (*checkAddr)(uint8_t, uint32_t, uint32_t),
                               uint32_t (*getAddr)(uint8_t, uint32_t),
                               uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount ) {
    bool ret = false;
    if( (*checkAddr)(SettingNumber, Address, ByteCount) ) {
        ret = FlashBufferedBytesWrite(Buffer, (*getAddr)(SettingNumber, Address), ByteCount);
    }
    return ret;
}

bool AppCardMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryWriteForSetting( &checkCardMemAddrConsistencyForSetting, &getCardMemFlashAddressForSetting,
                                     SettingNumber, Buffer, Address, ByteCount );
}

bool AppWorkingMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryWriteForSetting( &checkWorkingMemAddrConsistencyForSetting, &getWorkingMemFlashAddressForSetting,
                                     SettingNumber, Buffer, Address, ByteCount );
}

bool AppCardMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppCardMemoryWriteForSetting(GlobalSettings.ActiveSettingIdx, Buffer, Address, ByteCount);
}

bool AppWorkingMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppWorkingMemoryWriteForSetting(GlobalSettings.ActiveSettingIdx, Buffer, Address, ByteCount);
}

bool AppMemoryUploadXModem( uint32_t (*getSize)(void), bool (*memWrite)(const void*, uint32_t, uint32_t),
                            void* Buffer, uint32_t Address, uint32_t ByteCount ) {
    bool ret = false;
    uint32_t AvailBytes = (*getSize)();
    if(Address < AvailBytes) {
        uint32_t BytesLeft = MIN(ByteCount, AvailBytes - Address);
        ret = (*memWrite)((const void *)Buffer, Address, BytesLeft);
    }
    return ret;
}

bool AppCardMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryUploadXModem( &AppCardMemorySize, &AppCardMemoryWrite, Buffer, Address, ByteCount);
}

bool AppWorkingMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryUploadXModem( &AppWorkingMemorySize, &AppWorkingMemoryWrite, Buffer, Address, ByteCount);
}

/* Memory delete/clear operations
***************************************************************************************/

// Delete all memory
bool MemoryClearAll(void) {
    bool flashOK = FlashClearAll();
    bool eepromOK = EEPROMClearAll();
    return (flashOK && eepromOK);
}

bool AppSomeMemoryClearForSetting(uint8_t SettingNumber, uint32_t startAddress, uint32_t ByteCount) {
    bool ret = false;
    if( checkSettingNumberConsistency(SettingNumber) ) {
        ret = FlashClearRange( startAddress, ByteCount );
    }
    return ret;
}

bool AppMemoryClearForSetting(uint8_t SettingNumber) {
    return AppSomeMemoryClearForSetting(SettingNumber, getFlashAddressForSetting(SettingNumber, MEMORY_NO_ADDR), AppMemorySizeForSetting(SettingNumber));
}

bool AppMemoryClear(void) {
    return AppMemoryClearForSetting(GlobalSettings.ActiveSettingIdx);
}

bool AppCardMemoryClearForSetting(uint8_t SettingNumber) {
    return AppSomeMemoryClearForSetting(SettingNumber, getCardMemFlashAddressForSetting(SettingNumber, MEMORY_NO_ADDR), AppCardMemorySizeForSetting(SettingNumber));
}

bool AppWorkingMemoryClearForSetting(uint8_t SettingNumber) {
    return AppSomeMemoryClearForSetting(SettingNumber, getWorkingMemFlashAddressForSetting(SettingNumber, MEMORY_NO_ADDR), AppWorkingMemorySizeForSetting(SettingNumber));
}

bool AppCardMemoryClear(void) {
    return AppCardMemoryClearForSetting(GlobalSettings.ActiveSettingIdx);
}

bool AppWorkingMemoryClear(void) {
    return AppWorkingMemoryClearForSetting(GlobalSettings.ActiveSettingIdx);
}

bool AppMemoryUidMode(void) {
    return GlobalSettings.UidMode;
}
