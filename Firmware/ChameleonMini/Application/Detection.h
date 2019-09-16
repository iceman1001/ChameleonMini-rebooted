/*
 * Detection.h
 *
 * 2019, @shinhub
 *
 * Parts are from initial ChameleonMini-rebooted iceman1001's repo.
 */

#ifndef _CM_DETECTION_H_
#define _CM_DETECTION_H_

#include "Application.h"
#include "ISO14443-3A.h"

/*
 * This application generates Mifare Classic 1K compatible nonces in response to
 * a given reader's AUTH Phase 1 with KEY A or KEY B request.
 * It then storer this request, nonce, and subsequent reader's Phase 2 response to our
 * generated nonce, in a way that Proxmark3 "mfkey" will be able to operate.
 * This is supposed to allow mfkey to recover valid KEY A or KEY B for a given reader.
 * We map application memory so that BLOCK 0 is a valid Mifare Classic 1K one, while
 * rest of it stores mfkey's required data.
 * Rest of memory is evenly split between KEY A and KEY B AUTH attempts data.
 */

#define DETECTION_BLOCK0_ADDR               0
#define DETECTION_NONCE_SIZE                4         /* Bytes */
#define DETECTION_READER_AUTH_P1_SIZE       4
#define DETECTION_READER_AUTH_P2_SIZE       8
#define DETECTION_SAVE_P2_OFFSET            8

#define DETECTION_BYTES_PER_SAVE            16
#define DETECTION_MEM_BLOCK0_SIZE           16
#define DETECTION_MEM_DATA_START_ADDR       16
#define DETECTION_MEM_MAX_KEYX_SAVES        6
#define DETECTION_MEM_KEYTYPE_NUM           2
#define DETECTION_MEM_MFKEY_DATA_LEN        (DETECTION_MEM_MAX_KEYX_SAVES * DETECTION_BYTES_PER_SAVE * DETECTION_MEM_KEYTYPE_NUM)
#define DETECTION_MEM_KEYX_SEPARATOR_OFFSET (DETECTION_MEM_BLOCK0_SIZE + (DETECTION_MEM_MAX_KEYX_SAVES * DETECTION_BYTES_PER_SAVE))
#define DETECTION_MEM_APP_SIZE              (DETECTION_MEM_BLOCK0_SIZE + DETECTION_MEM_MFKEY_DATA_LEN) // 208 Bytes

#define DETECTION_1K_ATQA_VALUE             0x0004
#define DETECTION_1K_SAK_CL1_VALUE          0x08
#define DETECTION_UID_SIZE                  ISO14443A_UID_SIZE_SINGLE
#define DETECTION_UID_CL1_ADDRESS           0x00
#define DETECTION_UID_CL1_SIZE              ISO14443A_UID_SIZE_SINGLE

#define DETECTION_CMD_AUTH_A                0x60
#define DETECTION_CMD_AUTH_B                0x61
#define DETECTION_CMD_AUTH_FRAME_SIZE       2         /* Bytes without CRCA */
#define DETECTION_CMD_AUTH_RB_FRAME_SIZE    4         /* Bytes */


void MifareDetectionInit(void);
void MifareDetectionReset(void);
uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount);

#endif // _CM_DETECTION_H_
