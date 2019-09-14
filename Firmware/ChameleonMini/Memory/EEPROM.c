/*
 * EEPROM.c
 *
 * 2019, @shinhub
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include "EEPROM.h"
#include "../Common.h"

// Dummy init to prepare for future use
bool EEPROMInit(void) {
    bool ret = false;
    // Enable EEPROM data memory mapping
    NVM.CTRLB |= NVM_EEMAPEN_bm;
    // Calculate available EEPROM size per setting
    if (EEPROM_BYTES_TOTAL > EEPROM_NO_MEMORY) {
        EEPROMInfo.bytesTotal = EEPROM_BYTES_TOTAL;
        ret = true;
    }
    return ret;
}

bool EEPROMClearAll(void) {
    uint16_t addr;
    uint8_t clear[8] = { EEPROM_ERASE_ARRAY };
    uint8_t sreg;
    sreg = SREG;
    for( addr = EEPROM_START_ADDR; addr < EEPROM_BYTES_TOTAL; addr += EEPROM_ADDR_OFFSET ) {
        eeprom_update_block((const void*)&clear[0], (void*)addr, EEPROM_ADDR_OFFSET);
    }
    SREG = sreg;
    return true;
}
