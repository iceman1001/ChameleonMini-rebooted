/*
 * NTAG21x.h
 *
 *  Created on: 20.02.2019
 *      Author: gcammisa
 */

#ifndef NTAG21x_H_
#define NTAG21x_H_
#include "Application.h"
#include "ISO14443-3A.h"

//DEFINE ATQA and SAK
#define ATQA_VALUE 0x0044
#define SAK_VALUE 0x00

#define SAK_CL1_VALUE           ISO14443A_SAK_INCOMPLETE
#define SAK_CL2_VALUE           ISO14443A_SAK_COMPLETE_NOT_COMPLIANT

//ACK and NACK
#define ACK_VALUE                   0x0A
#define ACK_FRAME_SIZE          4 /* Bits */
#define NAK_INVALID_ARG             0x00
#define NAK_CRC_ERROR               0x01
#define NAK_NOT_AUTHED              0x04
#define NAK_EEPROM_ERROR            0x05
#define NAK_FRAME_SIZE          4

//DEFINING COMMANDS
/* ISO commands */
#define CMD_HALT 0x50
//NTAG COMMANDS
#define CMD_GET_VERSION 0x60
#define CMD_READ 0x30
#define CMD_FAST_READ 0x3A
#define CMD_WRITE 0xA2
#define CMD_COMPAT_WRITE 0xA0
#define CMD_READ_CNT 0x39
#define CMD_PWD_AUTH 0x1B
#define CMD_READ_SIG 0x3C


//MEMORY LAYOUT STUFF, addresses and sizes in bytes
//UID stuff
#define UID_CL1_ADDRESS         0x00
#define UID_CL1_SIZE            3
#define UID_BCC1_ADDRESS        0x03
#define UID_CL2_ADDRESS         0x04
#define UID_CL2_SIZE            4
#define UID_BCC2_ADDRESS        0x08
//LockBytes stuff
#define STATIC_LOCKBYTE_0_ADDRESS   0x0A
#define STATIC_LOCKBYTE_1_ADDRESS   0x0B
//CONFIG stuff
#define NTAG213_CONFIG_AREA_START_ADDRESS   NTAG21x_PAGE_SIZE * 0x29
#define NTAG215_CONFIG_AREA_START_ADDRESS   NTAG21x_PAGE_SIZE * 0x83
#define NTAG216_CONFIG_AREA_START_ADDRESS   NTAG21x_PAGE_SIZE * 0xE3
#define CONFIG_AREA_SIZE        8
//CONFIG offsets, relative to config start address
#define CONF_AUTH0_OFFSET       0x03
#define CONF_ACCESS_OFFSET      0x04
#define CONF_PASSWORD_OFFSET    0x08
#define CONF_PACK_OFFSET        0x0C

//WRITE STUFF
#define BYTES_PER_WRITE         4
#define PAGE_WRITE_MIN          0x02

//CONFIG masks to check individual needed bits
#define CONF_ACCESS_PROT        0x80

#define VERSION_INFO_LENGTH 8 //8 bytes info lenght + crc

#define BYTES_PER_READ NTAG21x_PAGE_SIZE * 4
//SIGNATURE Lenght
#define SIGNATURE_LENGTH        32

#define NTAG21x_UID_SIZE ISO14443A_UID_SIZE_DOUBLE //7 bytes UID

#define NTAG21x_PAGE_SIZE 4 //bytes per page
#define NTAG213_PAGES 45 //45 pages total for ntag213, from 0 to 44
#define NTAG215_PAGES 135 //135 pages total for ntag215, from 0 to 134
#define NTAG216_PAGES 231 //231 pages total for ntag216, from 0 to 230

#define NTAG213_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG213_PAGES )
#define NTAG215_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG215_PAGES )
#define NTAG216_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG216_PAGES )

void NTAG213AppInit(void);
void NTAG213AppReset(void);

void NTAG215AppInit(void);
void NTAG215AppReset(void);

void NTAG216AppInit(void);
void NTAG216AppReset(void);

uint16_t NTAG213AppProcess(uint8_t *Buffer, uint16_t BitCount);
uint16_t NTAG215AppProcess(uint8_t *Buffer, uint16_t BitCount);
uint16_t NTAG216AppProcess(uint8_t *Buffer, uint16_t BitCount);

void NTAG21xGetUid(ConfigurationUidType Uid);
void NTAG21xSetUid(ConfigurationUidType Uid);

uint8_t AppWritePage(uint8_t PageAddress, uint8_t *const Buffer);

#endif
