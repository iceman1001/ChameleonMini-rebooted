/*
 * MifareClassic.h
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#if defined(CONFIG_MF_CLASSIC_SUPPORT) || defined(CONFIG_MF_CLASSIC_DETECTION_SUPPORT) \
    || defined(CONFIG_MF_CLASSIC_BRUTE_SUPPORT) || defined(SUPPORT_MF_CLASSIC_MAGIC_MODE) \
    || defined(CONFIG_MF_CLASSIC_LOG_SUPPORT)

#ifndef MIFARECLASSIC_H_
#define MIFARECLASSIC_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define MFCLASSIC_UID_SIZE                      ISO14443A_UID_SIZE_SINGLE
#define MFCLASSIC_UID_7B_SIZE                   ISO14443A_UID_SIZE_DOUBLE
#define MFCLASSIC_1K_MEM_SIZE                   1024
#define MFCLASSIC_4K_MEM_SIZE                   4096
#define MFCLASSIC_MINI_MEM_SIZE                 320

#define MFCLASSIC_1K_ATQA_VALUE                 0x0004
#define MFCLASSIC_1K_7B_ATQA_VALUE              0x0044
#define MFCLASSIC_4K_ATQA_VALUE                 0x0002
#define MFCLASSIC_4K_7B_ATQA_VALUE              0x0042
#define MFCLASSIC_7B_ATQA_MASK                  0x40
#define MFCLASSIC_1K_SAK_VALUE                  0x08
#define MFCLASSIC_4K_SAK_VALUE                  0x18
#define MFCLASSIC_MINI_ATQA_VALUE               MFCLASSIC_1K_ATQA_VALUE
#define MFCLASSIC_MINI_SAK_VALUE                0x09
#define MFCLASSIC_SAK_CL1_VALUE                 ISO14443A_SAK_INCOMPLETE

#define MFCLASSIC_MEM_S0B0_ADDRESS              0x00
#define MFCLASSIC_MEM_INVALID_ADDRESS           0xFF
#define MFCLASSIC_MEM_UID_CL1_ADDRESS           0x00
#define MFCLASSIC_MEM_UID_CL1_SIZE              4
#define MFCLASSIC_MEM_UID_BCC1_ADDRESS          0x04
#define MFCLASSIC_MEM_UID_CL2_ADDRESS           0x03
#define MFCLASSIC_MEM_UID_CL2_SIZE              4
#define MFCLASSIC_MEM_KEY_A_OFFSET              48
#define MFCLASSIC_MEM_KEY_B_OFFSET              58
#define MFCLASSIC_MEM_KEY_BIGSECTOR_OFFSET      192
#define MFCLASSIC_MEM_KEY_SIZE                  6
#define MFCLASSIC_MEM_ACC_GPB_SIZE              4
#define MFCLASSIC_MEM_SECTOR_ADDR_MASK          0xFC
#define MFCLASSIC_MEM_BIGSECTOR_ADDR_MASK       0xF0
#define MFCLASSIC_MEM_BYTES_PER_BLOCK           16
#define MFCLASSIC_MEM_VALUE_SIZE                4
#define MFCLASSIC_MEM_NONCE_SIZE                4
#define MFCLASSIC_ACK_NAK_FRAME_SIZE            4
#define MFCLASSIC_ACK_VALUE                     0x0A

/* Unreferenced NAK values in RevG original code
#define NAK_INVALID_ARG                         0x00
#define NAK_CRC_ERROR                           0x01
#define NAK_NOT_AUTHED                          0x04
#define NAK_EEPROM_ERROR                        0x05
#define NAK_OTHER_ERROR                         0x06

NAK values from NXP MF1S50YYX_V1 datasheet, 9.3
Code (4-bit)|Transfer Buffer Validity|Description
-----------------------------------------------------------
Ah                                   |  Acknowledge (ACK)
0h          |  valid                 |  invalid operation
1h          |  valid                 |  parity or CRC error
4h          |  invalid               |  invalid operation
5h          |  invalid               |  parity or CRC error

NAK code conditions are not specified. As an internal convention and to ensure backward
compatibility with RevG applications, we only use TBKO constants when a buffer to read or
write is invalid, or if AUTH fails (TBKO_OPKO). Otherwise, TBOK_OPKO is used when an
operation is invalid, and TBOK_CRCKO for CRC or parity error in received frame.
*/
#define MFCLASSIC_NAK_TBOK_OPKO                 0x00
#define MFCLASSIC_NAK_TBOK_CRCKO                0x01
#define MFCLASSIC_NAK_TBKO_OPKO                 0x04
#define MFCLASSIC_NAK_TBKO_CRCKO                0x05

#define MFCLASSIC_CMD_WUPA_BITCOUNT             7        /* Bits */
#define MFCLASSIC_CMD_AUTH_A                    0x60
#define MFCLASSIC_CMD_AUTH_B                    0x61
#define MFCLASSIC_CMD_AUTH_FRAME_SIZE           2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_AUTH_RB_FRAME_SIZE        4        /* Bytes */
#define MFCLASSIC_CMD_AUTH_AB_FRAME_SIZE        8        /* Bytes */
#define MFCLASSIC_CMD_AUTH_BA_FRAME_SIZE        4        /* Bytes */
#define MFCLASSIC_CMD_HALT                      0x50
#define MFCLASSIC_CMD_HALT_FRAME_SIZE           2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_READ                      0x30
#define MFCLASSIC_CMD_READ_FRAME_SIZE           2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_READ_RESPONSE_FRAME_SIZE  16      /* Bytes without CRCA */
#define MFCLASSIC_CMD_WRITE                     0xA0
#define MFCLASSIC_CMD_WRITE_FRAME_SIZE          2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_DECREMENT                 0xC0
#define MFCLASSIC_CMD_DECREMENT_FRAME_SIZE      2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_INCREMENT                 0xC1
#define MFCLASSIC_CMD_INCREMENT_FRAME_SIZE      2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_RESTORE                   0xC2
#define MFCLASSIC_CMD_RESTORE_FRAME_SIZE        2        /* Bytes without CRCA */
#define MFCLASSIC_CMD_TRANSFER                  0xB0
#define MFCLASSIC_CMD_TRANSFER_FRAME_SIZE       2        /* Bytes without CRCA */
/* AUTH, HALT, READ, WRITE, INC/DECREMENT, TRANSFER and RESTORE all have same size */
#define MFCLASSIC_CMD_COMMON_FRAME_SIZE         2
/* Chinese magic backdoor commands (GEN 1A) */
#define MFCLASSIC_CMD_CHINESE_UNLOCK            0x40
#define MFCLASSIC_CMD_CHINESE_WIPE              0x41
#define MFCLASSIC_CMD_CHINESE_UNLOCK_RW         0x43



/*
Source: NXP: MF1S50YYX Product data sheet

Access conditions for the sector trailer

Access bits     Access condition for                   Remark
            KEYA         Access bits  KEYB
C1 C2 C3        read  write  read  write  read  write
0  0  0         never key A  key A never  key A key A  Key B may be read[1]
0  1  0         never never  key A never  key A never  Key B may be read[1]
1  0  0         never key B  keyA|B never never key B
1  1  0         never never  keyA|B never never never
0  0  1         never key A  key A  key A key A key A  Key B may be read,
                                                       transport configuration[1]
0  1  1         never key B  keyA|B key B never key B
1  0  1         never never  keyA|B key B never never
1  1  1         never never  keyA|B never never never

[1] For this access condition key B is readable and may be used for data
*/
#define MFCLASSIC_ACC_TRAILOR_READ_KEYA   0x01
#define MFCLASSIC_ACC_TRAILOR_WRITE_KEYA  0x02
#define MFCLASSIC_ACC_TRAILOR_READ_ACC    0x04
#define MFCLASSIC_ACC_TRAILOR_WRITE_ACC   0x08
#define MFCLASSIC_ACC_TRAILOR_READ_KEYB   0x10
#define MFCLASSIC_ACC_TRAILOR_WRITE_KEYB  0x20

/*
Access conditions for data blocks
Access bits Access condition for                 Application
C1 C2 C3     read     write     increment     decrement,
                                                transfer,
                                                restore

0 0 0         key A|B key A|B key A|B     key A|B     transport configuration
0 1 0         key A|B never     never         never         read/write block
1 0 0         key A|B key B     never         never         read/write block
1 1 0         key A|B key B     key B         key A|B     value block
0 0 1         key A|B never     never         key A|B     value block
0 1 1         key B     key B     never         never         read/write block
1 0 1         key B     never     never         never         read/write block
1 1 1         never     never     never         never         read/write block

*/
#define MFCLASSIC_ACC_BLOCK_READ      0x01
#define MFCLASSIC_ACC_BLOCK_WRITE     0x02
#define MFCLASSIC_ACC_BLOCK_INCREMENT 0x04
#define MFCLASSIC_ACC_BLOCK_DECREMENT 0x08

#define MFCLASSIC_KEY_A 0
#define MFCLASSIC_KEY_B 1

#define MFCLASSIC_BYTE_SWAP(x) (((uint8_t)(x)>>4)|((uint8_t)(x)<<4))
#define MFCLASSIC_ACC_NO_ACCESS 0x07

#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
#define DETECTION_BYTES_PER_SAVE                16
#define DETECTION_READER_AUTH_P1_SIZE           4
#define DETECTION_READER_AUTH_P2_SIZE           8
#define DETECTION_SAVE_P2_OFFSET                8
#define DETECTION_KEYX_SAVE_IDX                 0
#define DETECTION_MEM_BLOCK0_SIZE               16
#define DETECTION_MEM_DATA_START_ADDR           0x10
#define DETECTION_MEM_MAX_KEYX_SAVES            6
#define DETECTION_MEM_KEYTYPE_NUM               2
// (DETECTION_MEM_MAX_KEYX_SAVES * DETECTION_BYTES_PER_SAVE * DETECTION_MEM_KEYTYPE_NUM)
#define DETECTION_MEM_MFKEY_DATA_LEN            192
// (DETECTION_MEM_BLOCK0_SIZE + (DETECTION_MEM_MAX_KEYX_SAVES * DETECTION_BYTES_PER_SAVE))
#define DETECTION_MEM_KEYX_SEPARATOR_OFFSET     112
// (DETECTION_MEM_BLOCK0_SIZE + DETECTION_MEM_MFKEY_DATA_LEN)
#define DETECTION_MEM_APP_SIZE                  208
#define DETECTION_BLOCK0_CANARY                 0x52, 0x45, 0x56, 0x45, 0x56, 0x31, 0x2E, 0x34
#define DETECTION_BLOCK0_CANARY_ADDR            0x08
#define DETECTION_BLOCK0_CANARY_SIZE            8
#endif

#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
#define BRUTE_MEM_BRUTED_UID_ADDR               8
#define BRUTE_MEM_BRUTED_STATUS_CANARY          0xB1
#define BRUTE_MEM_BRUTED_STATUS_RESET           0xB0
#define BRUTE_MEM_BRUTED_STATUS_ADDR            0
#define BRUTE_MEM_BRUTED_STATUS_SIZE            1
#define BRUTE_WORKING_MEM_SIZE                  16
#define BRUTE_IDLE_MAX_ROUNDS                   3
#endif

#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
#define MFCLASSIC_LOG_LINE_START                0x3E
#define MFCLASSIC_LOG_LINE_END                  0x3B
#define MFCLASSIC_LOG_TAB                       0x09
#define MFCLASSIC_LOG_EOL_CR                    0x0D
#define MFCLASSIC_LOG_EOL_LF                    0x0A
#define MFCLASSIC_LOG_EOS                       0x00
#define MFCLASSIC_LOG_SEPARATOR                 0x21
#define MFCLASSIC_LOG_READER                    0x52
#define MFCLASSIC_LOG_TAG                       0x54
#define MFCLASSIC_LOG_MEM_CHAR_LEN              1
#define MFCLASSIC_LOG_MEM_STATUS_CANARY_ADDR    MFCLASSIC_LOG_MEM_LOG_HEADER_ADDR
#define MFCLASSIC_LOG_MEM_STATUS_CANARY         0x71
#define MFCLASSIC_LOG_MEM_STATUS_RESET          0x70
#define MFCLASSIC_LOG_MEM_STATUS_LEN            1
#define MFCLASSIC_LOG_MEM_WROTEBYTES_ADDR       12
#define MFCLASSIC_LOG_MEM_WROTEBYTES_LEN        sizeof(uint32_t)
#define MFCLASSIC_LOG_MEM_LOG_HEADER_ADDR       0
#define MFCLASSIC_LOG_MEM_LOG_HEADER_LEN        16
#define MFCLASSIC_LOG_MEM_LINE_BUFFER_LEN       256
#define MFCLASSIC_LOG_MEM_LINE_START_ADDR       0
#define MFCLASSIC_LOG_MEM_LINE_TIMESTAMP_LEN    sizeof(uint16_t)
#define MFCLASSIC_LOG_LINE_OVERHEAD             (MFCLASSIC_LOG_MEM_LINE_TIMESTAMP_LEN+MFCLASSIC_LOG_MEM_CHAR_LEN*10)
#define MFCLASSIC_LOG_BUFFER_OVERFLOW           0x0F
#endif

void MifareClassicAppInit1K(void);
void MifareClassicAppInit4K(void);
void MifareClassicAppInitMini(void);
void MifareClassicAppReset(void);

uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareClassicGetUid(ConfigurationUidType Uid);
void MifareClassicSetUid(ConfigurationUidType Uid);

void MifareClassicGetAtqa(uint16_t * Atqa);
void MifareClassicSetAtqa(uint16_t Atqa);

void MifareClassicGetSak(uint8_t * Sak);
void MifareClassicSetSak(uint8_t Sak);

#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
void MifareClassicAppDetectionInit(void);
#endif

#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
void MifareClassicAppBruteInit(void);
void MifareClassicAppBruteTick(void);
void MifareClassicAppBruteToggle(void);
#endif

#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
void MifareClassicAppLogInit(void);
void MifareClassicAppLogWriteLines(void);
void MifareClassicAppLogToggle(void);
#endif

#endif /* MIFARECLASSIC_H_ */

#endif /* Compilation support */
