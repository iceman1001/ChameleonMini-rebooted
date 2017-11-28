/*
 * Flash.h
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "Common.h"

#define MEMORY_SIZE					(FLASH_DATA_SIZE) /* From makefile */
#define MEMORY_INIT_VALUE			0x00

#define MEMORY_FLASH_USART	USARTD0
#define MEMORY_FLASH_PORT	PORTD
#define MEMORY_FLASH_CS		PIN4_bm
#define MEMORY_FLASH_MOSI	PIN3_bm
#define MEMORY_FLASH_MISO	PIN2_bm
#define MEMORY_FLASH_SCK	PIN1_bm

#define MEMORY_PAGE_SIZE		256
#define MEMORY_SIZE_PER_SETTING	((uint32_t) 256 * MEMORY_PAGE_SIZE) /* Multiple of memory page size */

int MemoryInit(void);
void MemoryReadBlock(void* Buffer, uint16_t Address, uint16_t ByteCount);
void MemoryWriteBlock(const void* Buffer, uint16_t Address, uint16_t ByteCount);
void MemoryClear(void);

/* For use with XModem */
bool MemoryUploadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);
bool MemoryDownloadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);

uint8_t Read_Save(void* Buffer, uint32_t Address, uint16_t ByteCount);
void Write_Save(void* Buffer, uint32_t Address, uint16_t ByteCount);
#endif /* MEMORY_H_ */
