/*
 * Memory.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#include "Memory.h"
#include "Configuration.h"
#include "Common.h"
#include "Settings.h"
#include "LED.h"

#define FLASH_CMD_READ					0x03
#define FLASH_CMD_STATUS_REG_READ		0xD7
#define FLASH_CMD_MEM_TO_BUF1			0x53
#define FLASH_CMD_MEM_TO_BUF2			0x55
#define FLASH_CMD_BUF1_WRITE			0x84
#define FLASH_CMD_BUF2_WRITE			0x87
#define FLASH_CMD_BUF1_TO_MEM_ERASE		0x83
#define FLASH_CMD_BUF2_TO_MEM_ERASE		0x86
#define FLASH_CMD_PAGE_ERASE			0x81

#define FLASH_STATUS_REG_READY_BIT		(1<<7)
#define FLASH_STATUS_REG_COMP_BIT		(1<<6)
#define FLASH_STATUS_REG_PROTECT_BIT	(1<<1)
#define FLASH_STATUS_REG_PAGESIZE_BIT	(1<<0)

INLINE uint8_t SPITransferByte(uint8_t Data)
{
	MEMORY_FLASH_USART.DATA = Data;

	while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));

	MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;

	return MEMORY_FLASH_USART.DATA;
}

INLINE void SPIReadBlock(void* Buffer, uint16_t ByteCount)
{
	uint8_t* ByteBuffer = (uint8_t*) Buffer;

	while(ByteCount-- > 0) {
		MEMORY_FLASH_USART.DATA = 0;
		while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));
		MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;
		*ByteBuffer++ = MEMORY_FLASH_USART.DATA;
	}
}

INLINE void SPIWriteBlock(const void* Buffer, uint16_t ByteCount)
{
	uint8_t* ByteBuffer = (uint8_t*) Buffer;

	while(ByteCount-- > 0) {
		MEMORY_FLASH_USART.DATA = *ByteBuffer++;
		while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));
		MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;
		MEMORY_FLASH_USART.DATA; /* Flush Buffer */
	}
}

INLINE uint8_t FlashReadStatusRegister(void)
{
	uint8_t Register;

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_STATUS_REG_READ);
	Register = SPITransferByte(0);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;

	return Register;
}

INLINE bool FlashIsBusy(void)
{
	return !(FlashReadStatusRegister() & FLASH_STATUS_REG_READY_BIT);
}

INLINE void FlashConfigurePageSize(void)
{
	uint8_t Sequence[] = {0x3D, 0x2A, 0x80, 0xA6};

	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPIWriteBlock(Sequence, sizeof(Sequence));
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashMemoryToBuffer(uint16_t PageAddress)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_MEM_TO_BUF1);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashWriteBuffer(const void* Buffer, uint8_t Address, uint16_t ByteCount)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_BUF1_WRITE);
	SPITransferByte( 0 );
	SPITransferByte( 0 );
	SPITransferByte( Address );
	SPIWriteBlock(Buffer, ByteCount);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashBufferToMemory(uint16_t PageAddress)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_BUF1_TO_MEM_ERASE);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashRead(void* Buffer, uint16_t Address, uint16_t ByteCount)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_READ);
	//SPITransferByte( (Address >> 16) & 0xFF );
	SPITransferByte( 0 );
	SPITransferByte( (Address >> 8) & 0xFF );
	SPITransferByte( (Address >> 0) & 0xFF );
	SPIReadBlock(Buffer, ByteCount);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashWrite(const void* Buffer, uint16_t Address, uint16_t ByteCount) {
	while(ByteCount > 0) {
		uint16_t PageAddress = Address / MEMORY_PAGE_SIZE;
		uint8_t ByteAddress = Address % MEMORY_PAGE_SIZE;
		uint16_t PageBytes = MIN(MEMORY_PAGE_SIZE - ByteAddress, ByteCount);

		FlashMemoryToBuffer(PageAddress);
		FlashWriteBuffer(Buffer, ByteAddress, PageBytes);
		FlashBufferToMemory(PageAddress);

		ByteCount -= PageBytes;
		Address += PageBytes;
	}
}

INLINE void FlashClearPage(uint16_t PageAddress) {
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_PAGE_ERASE);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

void MemoryInit(void) {
	/* Configure MEMORY_FLASH_USART for SPI master mode 0 with maximum clock frequency */
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
	
	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_SCK;
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_MOSI;
	
	MEMORY_FLASH_PORT.DIRSET = MEMORY_FLASH_SCK | MEMORY_FLASH_MOSI | MEMORY_FLASH_CS;

    MEMORY_FLASH_USART.BAUDCTRLA = 0;
    MEMORY_FLASH_USART.BAUDCTRLB = 0;
	MEMORY_FLASH_USART.CTRLC = USART_CMODE_MSPI_gc;
	MEMORY_FLASH_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;


	if ( !(FlashReadStatusRegister() & FLASH_STATUS_REG_PAGESIZE_BIT) ) {
		/* Configure for 256 byte Dataflash if not already done. */
		FlashConfigurePageSize();
	}
}

void MemoryReadBlock(void* Buffer, uint16_t Address, uint16_t ByteCount) {
	if (ByteCount == 0)
		return;

	uint32_t FlashAddress;
	if ( GlobalSettings.ActiveSetting == 0 )
		FlashAddress = (uint32_t) Address + (uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_4K;
	else
		FlashAddress = (uint32_t) Address + (uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_1K;

	FlashRead(Buffer, FlashAddress, ByteCount);
}

void MemoryWriteBlock(const void* Buffer, uint16_t Address, uint16_t ByteCount) {
	if (ByteCount == 0)
		return;
	uint16_t FlashAddress;
	if ( GlobalSettings.ActiveSetting == 0 )
		FlashAddress = Address + (uint16_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_4K;
	else
		FlashAddress = Address + (uint16_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_1K;
	FlashWrite(Buffer, FlashAddress, ByteCount);
}

void MemoryClear(void) {
	uint16_t PageAddress;
	uint16_t PageCount;
	if ( GlobalSettings.ActiveSetting == 0 ) {
		PageAddress = ((uint16_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_4K) / MEMORY_PAGE_SIZE;
		PageCount = MEMORY_SIZE_PER_SETTING_4K / MEMORY_PAGE_SIZE;
	} else {
		PageAddress = ((uint16_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING_1K) / MEMORY_PAGE_SIZE;
		PageCount = MEMORY_SIZE_PER_SETTING_1K / MEMORY_PAGE_SIZE;
	}

	while(PageCount > 0) {
		FlashClearPage(PageAddress);
		PageCount--;
		PageAddress++;
	}
}
void MemoryRecall(void) {
	/* Recall memory from permanent flash */
	//FlashRead(Memory, (uint16_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);
}

void MemoryStore(void) {
	/* Store current memory into permanent flash */
	// FlashWrite(Memory, (uint16_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);

	// LEDTrigger(LED_MEMORY_CHANGED, LED_OFF);
	// LEDTrigger(LED_MEMORY_STORED, LED_PULSE);
}

bool MemoryUploadBlock(void* Buffer, uint16_t BlockAddress, uint16_t ByteCount) {
	if ( GlobalSettings.ActiveSetting == 0 ) {
		if (BlockAddress >= MEMORY_SIZE_PER_SETTING_4K) {
        /* Prevent writing out of bounds by silently ignoring it */
        return true;
    } else {
    	/* Calculate bytes left in memory and start writing */
			uint16_t BytesLeft = MEMORY_SIZE_PER_SETTING_4K - BlockAddress;
		ByteCount = MIN(ByteCount, BytesLeft);
		MemoryWriteBlock(Buffer, BlockAddress, ByteCount);
		return true;
		}
	} else {
		if (BlockAddress >= MEMORY_SIZE_PER_SETTING_1K) {
			/* Prevent writing out of bounds by silently ignoring it */
			return true;
			} else {
			/* Calculate bytes left in memory and start writing */
			uint16_t BytesLeft = MEMORY_SIZE_PER_SETTING_1K - BlockAddress;
			ByteCount = MIN(ByteCount, BytesLeft);
			MemoryWriteBlock(Buffer, BlockAddress, ByteCount);
			return true;
		}
    }
}

bool MemoryDownloadBlock(void* Buffer, uint16_t BlockAddress, uint16_t ByteCount) {
	if ( GlobalSettings.ActiveSetting == 0 ) {
		if (BlockAddress >= MEMORY_SIZE_PER_SETTING_4K) {
        /* There are bytes out of bounds to be read. Notify that we are done. */
        return false;
    } else {
    	/* Calculate bytes left in memory and issue reading */
    		uint16_t BytesLeft = MEMORY_SIZE_PER_SETTING_4K - BlockAddress;
		ByteCount = MIN(ByteCount, BytesLeft);
    	MemoryReadBlock(Buffer, BlockAddress, ByteCount);
        return true;
		}
	} else {
		if (BlockAddress >= MEMORY_SIZE_PER_SETTING_1K) {
			/* There are bytes out of bounds to be read. Notify that we are done. */
			return false;
			} else {
			/* Calculate bytes left in memory and issue reading */
			uint16_t BytesLeft = MEMORY_SIZE_PER_SETTING_1K - BlockAddress;
			ByteCount = MIN(ByteCount, BytesLeft);
			MemoryReadBlock(Buffer, BlockAddress, ByteCount);
			return true;
		}
    }
}

// EEPROM functions
    
static inline void NVM_EXEC(void)
{   
    void *z = (void *)&NVM_CTRLA;

    __asm__ volatile("out %[ccp], %[ioreg]"  "\n\t"
    "st z, %[cmdex]"
    :
    : [ccp] "I" (_SFR_IO_ADDR(CCP)),
    [ioreg] "d" (CCP_IOREG_gc),
                 [cmdex] "r" (NVM_CMDEX_bm),
                 [z] "z" (z)
                 );
}
                 
void WaitForNVM(void)
{                
        while (NVM.STATUS & NVM_NVMBUSY_bm) { };
}
        
void FlushNVMBuffer(void)
{       
    WaitForNVM();

    if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
        NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
        NVM_EXEC();
    }
}

uint16_t ReadEEPBlock(uint16_t Address, void *DestPtr, uint16_t ByteCount)
{
    uint16_t BytesRead = 0;
    uint8_t* BytePtr = (uint8_t*) DestPtr;
    NVM.ADDR2 = 0;

    WaitForNVM();

    while (ByteCount > 0)
    {
            NVM.ADDR0 = Address & 0xFF;
            NVM.ADDR1 = (Address >> 8) & 0x1F;

            NVM.CMD = NVM_CMD_READ_EEPROM_gc;
            NVM_EXEC();

            *BytePtr++ = NVM.DATA0;
            Address++;

            ByteCount--;
            BytesRead++;
    }

    return BytesRead;
}


uint16_t WriteEEPBlock(uint16_t Address, const void *SrcPtr, uint16_t ByteCount)
{
    const uint8_t* BytePtr = (const uint8_t*) SrcPtr;
    uint8_t ByteAddress = Address % EEPROM_PAGE_SIZE;
    uint16_t PageAddress = Address - ByteAddress;
    uint16_t BytesWritten = 0;

    FlushNVMBuffer();
    WaitForNVM();
    NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

    NVM.ADDR1 = 0;
    NVM.ADDR2 = 0;

    while (ByteCount > 0)
    {
        NVM.ADDR0 = ByteAddress;

        NVM.DATA0 = *BytePtr++;

        ByteAddress++;
        ByteCount--;

        if (ByteCount == 0 || ByteAddress >= EEPROM_PAGE_SIZE)
        {
            NVM.ADDR0 = PageAddress & 0xFF;
            NVM.ADDR1 = (PageAddress >> 8) & 0x1F;

            NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
            NVM_EXEC();

            PageAddress += EEPROM_PAGE_SIZE;
            ByteAddress = 0;

            WaitForNVM();

            NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
        }

        BytesWritten++;
    }

    return BytesWritten;
}
