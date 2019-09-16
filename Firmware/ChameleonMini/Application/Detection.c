/*
 * Detection.c
 *
 * 2019, @shinhub
 *
 * Parts are from initial ChameleonMini-rebooted iceman1001's repo.
 */

#include <string.h>
#include <stdbool.h>
#include "Detection.h"
#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory/Memory.h"
#include "../Random.h"
#include "../Settings.h"

static enum {
    STATE_HALT,
    STATE_IDLE,
    STATE_READY,
    STATE_ACTIVE,
    STATE_AUTHING,
} State;

static uint16_t CardATQAValue;
static uint8_t CardSAKValue;
static uint8_t data_save[DETECTION_BYTES_PER_SAVE] = {0};
static uint8_t turn_flaga = 0;
static uint8_t turn_flagb = 0;
static bool keyb_flag = false;

uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    /* Size of data (byte) we will send back to reader. Is main process retSize = value */
    uint16_t retSize = ISO14443A_APP_NO_RESPONSE;
    /* Wakeup and Request may occur in all states */
    if ( (BitCount == 7) &&
    /* precheck of WUP/REQ because ISO14443AWakeUp destroys BitCount */
    (((State != STATE_HALT) && (Buffer[0] == ISO14443A_CMD_REQA)) ||
    (Buffer[0] == ISO14443A_CMD_WUPA) )) {
        if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
            State = STATE_READY;
            retSize = BitCount;
        }
    }

    switch(State) {
        case STATE_IDLE:
        case STATE_HALT:
            if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, true)) {
                State = STATE_READY;
                retSize = BitCount;
            }
        break;

        case STATE_READY:
            if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
                State = STATE_READY;
                retSize = BitCount;
            } else if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
                /* Load UID CL1 and perform anti-collision */
                uint8_t UidCL1[4];
                AppMemoryRead(UidCL1, DETECTION_UID_CL1_ADDRESS, DETECTION_UID_CL1_SIZE);
                if (ISO14443ASelect(Buffer, &BitCount, UidCL1, CardSAKValue)) {
                    State = STATE_ACTIVE;
                }
                retSize = BitCount;
            } else {
                /* Unknown command. Enter HALT state. */
                State = STATE_HALT;
            }
        break;

        case STATE_ACTIVE:
            if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
                State = STATE_READY;
                retSize = BitCount;
            } else if ( (Buffer[0] == DETECTION_CMD_AUTH_A) || (Buffer[0] == DETECTION_CMD_AUTH_B)) {
                if (ISO14443ACheckCRCA(Buffer, DETECTION_CMD_AUTH_FRAME_SIZE)) {
                    // Save reader's auth phase 1: KEY type (A or B), and sector number
                    memcpy(data_save, Buffer, DETECTION_READER_AUTH_P1_SIZE);
                    // Generate a random nonce and save it for later output to mfkey
                    RandomGetBuffer(data_save+DETECTION_READER_AUTH_P1_SIZE, DETECTION_NONCE_SIZE);
                    // Save if auth was with KEY B, to properly build output in STATE_AUTHING
                    keyb_flag = (Buffer[0] == DETECTION_CMD_AUTH_B);
                    // Fill reply buffer with generated nonce
                    memcpy(Buffer, data_save, DETECTION_NONCE_SIZE);
                    // Pretend success
                    State = STATE_AUTHING;
                    retSize = DETECTION_CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
                }
            }
        break;

        case STATE_AUTHING:
            // Save reader's auth phase 2 answer to our nonce from STATE_ACTIVE
            memcpy(data_save+DETECTION_SAVE_P2_OFFSET, Buffer, DETECTION_READER_AUTH_P2_SIZE);
            uint16_t memSaveAddr;
            // Separated KEY A and KEY B AUTH data by DETECTION_MEM_KEYX_SEPARATOR_OFFSET bytes
            if (!keyb_flag) {
                memSaveAddr = DETECTION_MEM_BLOCK0_SIZE + (turn_flaga * DETECTION_BYTES_PER_SAVE);
                turn_flaga++;
                turn_flaga = turn_flaga % DETECTION_MEM_MAX_KEYX_SAVES;
            } else {
                memSaveAddr = DETECTION_MEM_BLOCK0_SIZE + DETECTION_MEM_KEYX_SEPARATOR_OFFSET + (turn_flagb * DETECTION_BYTES_PER_SAVE);
                turn_flagb++;
                turn_flagb = turn_flagb % DETECTION_MEM_MAX_KEYX_SAVES;
            }
            AppMemoryWrite(data_save, memSaveAddr, DETECTION_BYTES_PER_SAVE);
            // Leave to IDLE state, aka rage quit
            State = STATE_IDLE;
        break;

        default:
            break;
    } // End switch/case

    return retSize;
}

void MifareDetectionInit(void) {
    State = STATE_IDLE;
    CardATQAValue = DETECTION_1K_ATQA_VALUE;
    CardSAKValue = DETECTION_1K_SAK_CL1_VALUE;
}

void MifareDetectionReset(void) {
    State = STATE_IDLE;
}
