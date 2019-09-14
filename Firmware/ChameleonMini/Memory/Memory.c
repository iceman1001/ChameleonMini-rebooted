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

// Set after correct memory init
static bool isMemoryInit = false;

/* Common helpers for memory operations
***************************************************************************************/

// How much memory a setting's application will take. If set to 0 or to more than possible,
// pretend that max available memory for a setting/slot is used.
// Otherwise, give what is required by setting's configuration value 'MemorySize'.
INLINE uint32_t getAppMemSizeForSetting(uint8_t SettingNumber) {
    uint16_t requiredMem = ConfigurationTableGetMemorySizeForId(GlobalSettings.Settings[SettingNumber].Configuration);
    if( (requiredMem == MEMORY_NO_MEMORY) || (requiredMem > MemoryMappingInfo.maxFlashBytesPerSlot) ) {
        requiredMem = MemoryMappingInfo.maxFlashBytesPerSlot;
    }
    return requiredMem;
}

INLINE bool checkSettingNumberConsistency(uint8_t SettingNumber) {
    return ( (SettingNumber >= SETTINGS_FIRST) && (SettingNumber <= SETTINGS_LAST) );
}

// Is an address start + offset R/W operation valid in setting's application memory space
INLINE bool checkAddrConsistencyForSetting(uint8_t SettingNumber, uint32_t Address, uint32_t ByteCount) {
    bool ret = (isMemoryInit && checkSettingNumberConsistency(SettingNumber));
    if(ret) {
        ret = (Address <= (getAppMemSizeForSetting(SettingNumber) - ByteCount));
    }
    return ret;
}

// Returns a byte address in SPI Flash from a byte address relative to application's
// memory space.
// Does not check for address validity in application's space (checkSettingAddrConsistency
// must be used if needed).
INLINE uint32_t getFlashAddressForSetting(uint8_t SettingNumber, uint32_t Address) {
    return ((uint32_t)(SettingNumber * MemoryMappingInfo.maxFlashBytesPerSlot) + Address);
}

/* Memory init operations
***************************************************************************************/

// Init memory mapping and return false if not enough memory or not supported chip.
bool MemoryInit(void) {
    bool flashOk = false;
    bool eepromOk = false;
    if( FlashInit() ) {
        MemoryMappingInfo.maxFlashBytesPerSlot = (FlashInfo.geometry.sizeBytes / SETTINGS_COUNT);
        if( MemoryMappingInfo.maxFlashBytesPerSlot >= MEMORY_MIN_BYTES_PER_APP ) {
            flashOk = true;
        } else {
            MemoryMappingInfo.maxFlashBytesPerSlot = MEMORY_NO_MEMORY;
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
    isMemoryInit = (flashOk && eepromOk);
    return isMemoryInit;
}

/* Memory read operations
***************************************************************************************/

bool AppMemoryReadForSetting(uint8_t SettingNumber, void* Buffer, uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    if( checkAddrConsistencyForSetting(SettingNumber, Address, ByteCount) ) {
        ret = FlashUnbufferedBytesRead(Buffer, getFlashAddressForSetting(SettingNumber, Address), ByteCount);
    }
    return ret;
}

bool AppMemoryRead(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryReadForSetting(GlobalSettings.ActiveSetting, Buffer, Address, ByteCount);
}

bool AppMemoryDownloadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryRead(Buffer, Address, ByteCount);
}

/* Memory write operations
***************************************************************************************/

bool AppMemoryWriteForSetting(uint8_t SettingNumber, const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    bool ret = false;
    if( checkAddrConsistencyForSetting(SettingNumber, Address, ByteCount) ) {
        ret = FlashBufferedBytesWrite(Buffer, getFlashAddressForSetting(SettingNumber, Address), ByteCount);
    }
    return ret;
}

bool AppMemoryWrite(const void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryWriteForSetting(GlobalSettings.ActiveSetting, Buffer, Address, ByteCount);
}

bool AppMemoryUploadXModem(void* Buffer, uint32_t Address, uint32_t ByteCount) {
    return AppMemoryWrite((const void *)Buffer, Address, ByteCount);
}

/* Memory delete/clear operations
***************************************************************************************/

// Delete all memory
bool MemoryClearAll(void) {
    bool flashOK = FlashClearAll();
    bool eepromOK = EEPROMClearAll();
    return (flashOK && eepromOK);
}

bool AppMemoryClearForSetting(uint8_t SettingNumber) {
    bool ret = false;
    if( checkSettingNumberConsistency(SettingNumber) ) {
        ret = FlashClearRange( getFlashAddressForSetting(SettingNumber, MEMORY_NO_ADDR), getAppMemSizeForSetting(SettingNumber) );
    }
    return ret;
}

bool AppMemoryClear(void) {
    return AppMemoryClearForSetting(GlobalSettings.ActiveSetting);
}
