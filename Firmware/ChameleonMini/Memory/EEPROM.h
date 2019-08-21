/*
 * EEPROM.h
 *
 * 2019, @shinhub
 *
 */

#ifndef _MEM_EEPROM_H_
#define _MEM_EEPROM_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#define EEPROM_BYTES_TOTAL          (E2END + 1)
#define EEPROM_NO_MEMORY            0x00
#define EEPROM_START_ADDR           0
#define EEPROM_ERASE_ARRAY          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
#define EEPROM_ERASE_BYTE           0xFF
#define EEPROM_ADDR_OFFSET          8 // Bytes

typedef struct {
    uint16_t bytesTotal;
} EEPROMInfo_t;

EEPROMInfo_t EEPROMInfo;

bool EEPROMClearAll(void);
bool EEPROMInit(void);

#endif /* _MEM_EEPROM_H_ */
