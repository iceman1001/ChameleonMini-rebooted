/*
 * MifareClassic.c
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#include "MifareClassic.h"
#include "ISO14443-3A.h"
#include "Crypto1.h"
#include "../Random.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory/Memory.h"

/* TODO: Access control not implemented yet
// Decoding table for Access conditions of a data block
static const uint8_t abBlockAccessConditions[8][2] =
{
    //C1C2C3
    // 0 0 0 R:key A|B W: key A|B I:key A|B D:key A|B   transport configuration
    {
        // Access with Key A
        MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT
    },
    // 1 0 0 R:key A|B W:key B I:never D:never  read/write block
    {
        // Access with Key A
        MFCLASSIC_ACC_BLOCK_READ,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE
    },
    // 0 1 0 R:key A|B W:never I:never D:never  read/write block
    {
        // Access with Key A
        MFCLASSIC_ACC_BLOCK_READ,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ
    },
    // 1 1 0 R:key A|B W:key B I:key B D:key A|B    value block
    {
        // Access with Key A
        MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT
    },
    // 0 0 1 R:key A|B W:never I:never D:key A|B    value block
    {
        // Access with Key A
        MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT
    },
    // 1 0 1 R:key B W:never I:never D:never    read/write block
    {
        // Access with Key A
        0,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ
    },
    // 0 1 1 R:key B W:key B I:never D:never    read/write block
    {
        // Access with Key A
        0,
        // Access with Key B
        MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE
    },
    // 1 1 1 R:never W:never I:never D:never    read/write block
    {
        // Access with Key A
        0,
        // Access with Key B
        0
    }

};
// Decoding table for Access conditions of the sector trailor
static const uint8_t abTrailorAccessConditions[8][2] =
{
    // 0  0  0 RdKA:never WrKA:key A  RdAcc:key A WrAcc:never  RdKB:key A WrKB:key A    Key B may be read[1]
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB,
        // Access with Key B
        0
    },
    // 1  0  0 RdKA:never WrKA:key B  RdAcc:keyA|B WrAcc:never RdKB:never WrKB:key B
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        // Access with Key B
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC |  MFCLASSIC_ACC_TRAILOR_WRITE_KEYB
    },
    // 0  1  0 RdKA:never WrKA:never  RdAcc:key A WrAcc:never  RdKB:key A WrKB:never  Key B may be read[1]
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB,
        // Access with Key B
        0
    },
    // 1  1  0         never never  keyA|B never never never
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        // Access with Key B
        MFCLASSIC_ACC_TRAILOR_READ_ACC
    },
    // 0  0  1         never key A  key A  key A key A key A  Key B may be read,transport configuration[1]
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB,
        // Access with Key B
        0
    },
    // 0  1  1         never key B  keyA|B key B never key B
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        // Access with Key B
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB
    },
    // 1  0  1         never never  keyA|B key B never never
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        // Access with Key B
        MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC
    },
    // 1  1  1         never never  keyA|B never never never
    {
        // Access with Key A
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        // Access with Key B
        MFCLASSIC_ACC_TRAILOR_READ_ACC
    },
};
*/

enum estate {
    STATE_HALT,
    STATE_IDLE,
    STATE_CHINESE_IDLE,
    STATE_CHINESE_WRITE,
    STATE_READY1,
    STATE_READY2,
    STATE_ACTIVE,
    STATE_AUTHING,
    STATE_AUTHED_IDLE,
    STATE_WRITE,
    STATE_INCREMENT,
    STATE_DECREMENT,
    STATE_RESTORE
};
/* Init to get sure we have a controlled value wherever we start */
static enum estate State = STATE_IDLE;

static uint8_t CardResponse[MFCLASSIC_MEM_NONCE_SIZE];
static uint8_t ReaderResponse[MFCLASSIC_MEM_NONCE_SIZE];
static uint8_t CurrentAddress;
static uint8_t BlockBuffer[MFCLASSIC_MEM_BYTES_PER_BLOCK];
/* TODO: Access control not implemented yet
static uint8_t AccessConditions[MFCLASSIC_MEM_ACC_GPB_SIZE]; // Access Conditions + General purpose Byte
static uint8_t AccessAddress;
*/
static uint16_t CardATQAValue;
static uint8_t CardSAKValue;
static bool is7BytesUID = false;
/* To differentiate between IDLE and HALT, and to check for valid WUPA or REQA */
static bool isFromHaltState = false;
/* To enable MF_DETECTION behavior */
static bool isDetectionEnabled = false;
#ifdef CONFIG_MF_DETECTION_SUPPORT
static uint8_t DetectionDataSave[DETECTION_BYTES_PER_SAVE] = {0};
static uint8_t DetectionAttemptsKeyA = 0;
static uint8_t DetectionAttemptsKeyB = 0;
#endif

/* TODO: Access control not implemented yet
* Decode Access conditions for a block
INLINE uint8_t GetAccessCondition(uint8_t Block)
{
    uint8_t  InvSAcc0;
    uint8_t  InvSAcc1;
    uint8_t  Acc0 = AccessConditions[0];
    uint8_t  Acc1 = AccessConditions[1];
    uint8_t  Acc2 = AccessConditions[2];
    uint8_t  ResultForBlock = 0;

    InvSAcc0 = ~MFCLASSIC_BYTE_SWAP(Acc0);
    InvSAcc1 = ~MFCLASSIC_BYTE_SWAP(Acc1);

    // Check
    if ( ((InvSAcc0 ^ Acc1) & 0xf0) ||   // C1x
         ((InvSAcc0 ^ Acc2) & 0x0f) ||   // C2x
         ((InvSAcc1 ^ Acc2) & 0xf0))     // C3x
    {
      return(ACC_NO_ACCESS);
    }
    // Fix for MFClassic 4K cards
    if(Block<128)
        Block &= 3;
    else {
        Block &= 15;
        if (Block& 15)
            Block=3;
        else if (Block<=4)
            Block=0;
        else if (Block<=9)
            Block=1;
        else
            Block=2;
    }

    Acc0 = ~Acc0;       // C1x Bits to bit 0..3
    Acc1 =  Acc2;       // C2x Bits to bit 0..3
    Acc2 =  Acc2 >> 4;  // C3x Bits to bit 0..3

    if(Block)
    {
        Acc0 >>= Block;
        Acc1 >>= Block;
        Acc2 >>= Block;
    }
    // Combine the bits
    ResultForBlock = ((Acc2 & 1) << 2) |
                     ((Acc1 & 1) << 1) |
                     (Acc0 & 1);
    return(ResultForBlock);
}
*/


INLINE bool CheckValueIntegrity(uint8_t* Block) {
    // Value Blocks contain a value stored three times, with
    // the middle portion inverted.
    if (    (Block[0] == (uint8_t) ~Block[4]) && (Block[0] == Block[8])
         && (Block[1] == (uint8_t) ~Block[5]) && (Block[1] == Block[9])
         && (Block[2] == (uint8_t) ~Block[6]) && (Block[2] == Block[10])
         && (Block[3] == (uint8_t) ~Block[7]) && (Block[3] == Block[11])
         && (Block[12] == (uint8_t) ~Block[13])
         && (Block[12] == Block[14])
         && (Block[14] == (uint8_t) ~Block[15])) {
        return true;
    } else {
        return false;
    }
}

INLINE void ValueFromBlock(uint32_t* Value, uint8_t* Block) {
    *Value = 0;
    *Value |= ((uint32_t) Block[0] << 0);
    *Value |= ((uint32_t) Block[1] << 8);
    *Value |= ((uint32_t) Block[2] << 16);
    *Value |= ((uint32_t) Block[3] << 24);
}

INLINE void ValueToBlock(uint8_t* Block, uint32_t Value) {
    Block[0] = (uint8_t) (Value >> 0);
    Block[1] = (uint8_t) (Value >> 8);
    Block[2] = (uint8_t) (Value >> 16);
    Block[3] = (uint8_t) (Value >> 24);
    Block[4] = ~Block[0];
    Block[5] = ~Block[1];
    Block[6] = ~Block[2];
    Block[7] = ~Block[3];
    Block[8] = Block[0];
    Block[9] = Block[1];
    Block[10] = Block[2];
    Block[11] = Block[3];
}


void MifareClassicAppInit(uint16_t ATQA_4B, uint8_t SAK, bool is7B, bool isDetection) {
    State = STATE_IDLE;
    is7BytesUID = is7B;
    uint16_t expectedATQA = (is7BytesUID) ? (ATQA_4B | MFCLASSIC_7B_ATQA_MASK) : (ATQA_4B);
    uint8_t expectedSAK = SAK;
    uint16_t readATQA;
    uint8_t readSAK;
    MifareClassicGetSak(&readSAK);
    MifareClassicGetAtqa(&readATQA);
    if ( (readSAK == MFCLASSIC_MEM_SAK_VOID) ) {
        MifareClassicSetSak(expectedSAK);
    }
    if ( (readATQA == MFCLASSIC_MEM_ATQA_VOID) ) {
        MifareClassicSetAtqa(expectedATQA);
    }
    isFromHaltState = false;
    isDetectionEnabled = isDetection;
    if(isDetectionEnabled) {
        uint8_t canary[DETECTION_BLOCK0_CANARY_SIZE] = { DETECTION_BLOCK0_CANARY };
        AppMemoryWrite(canary, DETECTION_BLOCK0_CANARY_ADDR, DETECTION_BLOCK0_CANARY_SIZE);
    }
}

void MifareClassicAppInit1K(void) {
    MifareClassicAppInit( MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE,
                          (ActiveConfiguration.UidSize == MFCLASSIC_UID_7B_SIZE), false );
}

void MifareClassicAppInit4K(void) {
    MifareClassicAppInit( MFCLASSIC_4K_ATQA_VALUE, MFCLASSIC_4K_SAK_VALUE,
                          (ActiveConfiguration.UidSize == MFCLASSIC_UID_7B_SIZE), false );
}

#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
void MifareClassicAppInitMini(void)
{
    MifareClassicAppInit( MFCLASSIC_MINI_ATQA_VALUE, MFCLASSIC_MINI_SAK_VALUE,
                          false, false );
}
#endif

#ifdef CONFIG_MF_DETECTION_SUPPORT
void MifareClassicAppDetectionInit(void) {
    MifareClassicAppInit( MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE,
                          false, true );
}
#endif

void MifareClassicAppReset(void)
{
    State = STATE_IDLE;
}

void MifareClassicAppTask(void)
{

}


/* Handle a MFCLASSIC_CMD_HALT during main process, as can be raised in many states.
* Sets State, isFromHaltState, response buffer and returns response size */
uint16_t mfcHandleHaltCommand(uint8_t* Buffer) {
    uint16_t ret = MFCLASSIC_ACK_NAK_FRAME_SIZE;
    /* Halts the tag. According to the ISO14443, the second byte is supposed to be 0 */
    if (Buffer[1] == 0) {
        if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_HALT_FRAME_SIZE)) {
            /* According to ISO14443, we must not send anything to ACK */
            State = STATE_HALT;
            isFromHaltState = true;
            ret = ISO14443A_APP_NO_RESPONSE;
        } else {
            Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
        }
    } else {
        Buffer[0] = MFCLASSIC_NAK_TBOK_OPKO;
    }
    return ret;
}

/* Handle a WUPA or REQA during main process, as can be raised in all states.
* Sets State, response buffer and response size. Returns if WUPA/REQA received. */
uint16_t mfcHandleWUPCommand(uint8_t* Buffer, uint16_t BitCount, uint16_t ATQAValue, uint16_t * RetValue) {
    bool ret = false;
    /* If we have been awoken */
    if ( (BitCount == MFCLASSIC_CMD_WUPA_BITCOUNT) && ISO14443AIsWakeUp(Buffer, isFromHaltState) ) {
        /* Set response buffer */
        ISO14443ASetWakeUpResponse(Buffer, ATQAValue);
        /* We should go to HALT or IDLE depending on previous state if we are
        * in a ACTIVE, READY1 or READY2 */
        if ( (State == STATE_READY1) || (State == STATE_READY2) || (State == STATE_ACTIVE) ) {
            State = isFromHaltState ? STATE_HALT : STATE_IDLE;
            *RetValue = ISO14443A_APP_NO_RESPONSE;
        } else {
            /* Not implemented yet. AccessAddress = MFCLASSIC_MEM_INVALID_ADDRESS; */
            State = STATE_READY1;
            *RetValue = ISO14443A_ATQA_FRAME_SIZE;
        }
        ret = true;
    }
    return ret;
}

uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount) {
    /* Size of data (byte) we will send back to reader. Is main process return value */
    uint16_t retSize = ISO14443A_APP_NO_RESPONSE;
    /* Reset Halt state */
    isFromHaltState = (State == STATE_HALT);
    /* Wakeup and Request may occure in all states. We handle is first, so we can skip
    * states cases if we get valid WUPA/REQA */
    if (!mfcHandleWUPCommand(Buffer, BitCount, CardATQAValue, &retSize)) {
        switch(State) {
        case STATE_IDLE:
        case STATE_HALT:
            //isFromHaltState = (State == STATE_HALT);
#ifdef SUPPORT_MF_CLASSIC_MAGIC_MODE
            if (Buffer[0] == MFCLASSIC_CMD_CHINESE_UNLOCK) {
                State = STATE_CHINESE_IDLE;
                Buffer[0] = MFCLASSIC_ACK_VALUE;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            }
#endif
            break; /* End of states IDLE and HALT */

#ifdef SUPPORT_MF_CLASSIC_MAGIC_MODE
        case STATE_CHINESE_IDLE:
            /* Support special china commands that dont require authentication. */
            if (Buffer[0] == MFCLASSIC_CMD_CHINESE_UNLOCK_RW) {
                /* Unlock read and write commands */
                Buffer[0] = MFCLASSIC_ACK_VALUE;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            } else if (Buffer[0] == MFCLASSIC_CMD_CHINESE_WIPE) {
                /* Wipe memory */
                Buffer[0] = MFCLASSIC_ACK_VALUE;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            } else if ( (Buffer[0] == MFCLASSIC_CMD_READ) || (Buffer[0] == MFCLASSIC_CMD_WRITE) ) {
                if (!ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_COMMON_FRAME_SIZE)) {
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                } else if (Buffer[0] == MFCLASSIC_CMD_READ) {
                    /* Read command. Read data from memory and append CRCA. */
                    AppMemoryRead(Buffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                    ISO14443AAppendCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                    retSize = (MFCLASSIC_CMD_READ_RESPONSE_FRAME_SIZE + ISO14443A_CRCA_SIZE)
                              * BITS_PER_BYTE;
                } else if (Buffer[0] == MFCLASSIC_CMD_WRITE) {
                    /* Write command. Store the address and prepare for the upcoming data.
                    * Respond with ACK. */
                    CurrentAddress = Buffer[1];
                    State = STATE_CHINESE_WRITE;
                    Buffer[0] = MFCLASSIC_ACK_VALUE;
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                }
            } else if (Buffer[0] == MFCLASSIC_CMD_HALT) {
               retSize = mfcHandleHaltCommand(Buffer);
            }
            break; /* End of state CHINESE_IDLE */

        case STATE_CHINESE_WRITE:
            if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK)) {
                /* CRC check passed. Write data into memory and send ACK. */
                if (!ActiveConfiguration.ReadOnly) {
                    AppMemoryWrite(Buffer, CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                }
                Buffer[0] = MFCLASSIC_ACK_VALUE;
            } else {
                /* CRC Error. */
                Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
            }
            State = STATE_CHINESE_IDLE;
            retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            break;
#endif

        case STATE_READY1:
            if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
                /* Load UID CL1 and perform anticollision */
                uint8_t UidCL1[ISO14443A_CL_UID_SIZE];
                if (is7BytesUID) {
                    AppMemoryRead(&UidCL1[1], MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE-1);
                    UidCL1[0] = ISO14443A_UID0_CT;
                    if (ISO14443ASelect(Buffer, &BitCount, UidCL1, MFCLASSIC_SAK_CL1_VALUE)) {
                        State = STATE_READY2;
                    }
                } else {
                    AppMemoryRead(UidCL1, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
                    if (ISO14443ASelect(Buffer, &BitCount, UidCL1, CardSAKValue)) {
                        /* TODO: Access control not implemented yet
                        * AccessAddress = MFCLASSIC_MEM_INVALID_ADDRESS; // invalid, force reload */
                        State = STATE_ACTIVE;
                    }
                }
                /* Will be frame size if selected, or 0 else, as set by ISO14443ASelect */
                retSize = BitCount;
            } else {
                /* Unknown command. Enter HALT state. */
                State = STATE_HALT;
            }
            break; /* End of state READY1 */

        case STATE_READY2:
            if (Buffer[0] == ISO14443A_CMD_SELECT_CL2) {
               /* Load UID CL2 and perform anticollision */
               uint8_t UidCL2[ISO14443A_CL_UID_SIZE];
               AppMemoryRead(UidCL2, MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
               if (ISO14443ASelect(Buffer, &BitCount, UidCL2, CardSAKValue)) {
                   /* TODO: Access control not implemented yet
                   AccessAddress = MFCLASSIC_MEM_INVALID_ADDRESS; // invalid, force reload */
                   State = STATE_ACTIVE;
               }
               /* Will be frame size if selected, or 0 else, as set by ISO14443ASelect */
               retSize = BitCount;
            } else {
              /* Unknown command. Enter HALT state. */
              State = STATE_HALT;
            }
            break; /* End of state READY2 */

        case STATE_ACTIVE:
            if (Buffer[0] == MFCLASSIC_CMD_HALT) {
                retSize = mfcHandleHaltCommand(Buffer);
            } else if ( (Buffer[0] == MFCLASSIC_CMD_AUTH_A) || (Buffer[0] == MFCLASSIC_CMD_AUTH_B) ) {
                if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_AUTH_FRAME_SIZE)) {
                    uint8_t CardNonce[MFCLASSIC_MEM_NONCE_SIZE] = {0x01,0x20,0x01,0x45};
                    if(isDetectionEnabled) {
                        // Generate random nonce
                        RandomGetBuffer(CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
                        // Save reader's auth phase 1: KEY type (A or B), and sector number
                        memcpy(DetectionDataSave, Buffer, DETECTION_READER_AUTH_P1_SIZE);
                        // Save sent 'random' nonce
                        memcpy(DetectionDataSave+DETECTION_READER_AUTH_P1_SIZE, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
                    } else {
                        uint8_t SectorAddress;
                        uint8_t KeyOffset = (Buffer[0] == MFCLASSIC_CMD_AUTH_A ? MFCLASSIC_MEM_KEY_A_OFFSET : MFCLASSIC_MEM_KEY_B_OFFSET);
                        /* Fix for MFClassic 4k cards */
                        if(Buffer[1] >= 128) {
                            SectorAddress = Buffer[1] & MFCLASSIC_MEM_BIGSECTOR_ADDR_MASK;
                            KeyOffset += MFCLASSIC_MEM_KEY_BIGSECTOR_OFFSET;
                        } else {
                            SectorAddress = Buffer[1] & MFCLASSIC_MEM_SECTOR_ADDR_MASK;
                        }
                        uint16_t KeyAddress = (uint16_t) SectorAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK + KeyOffset;
                        uint8_t Key[MFCLASSIC_MEM_KEY_SIZE];
                        uint8_t Uid[MFCLASSIC_UID_SIZE];
                        uint8_t CardNonceSuccessor1[MFCLASSIC_MEM_NONCE_SIZE] = {0x63, 0xe5, 0xbc, 0xa7};
                        uint8_t CardNonceSuccessor2[MFCLASSIC_MEM_NONCE_SIZE] = {0x99, 0x37, 0x30, 0xbd};
                        /* Generate a random nonce and read UID and key from memory */
                        //RandomGetBuffer(CardNonce, sizeof(CardNonce));
                        if (is7BytesUID) {
                            AppMemoryRead(Uid, MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
                        } else {
                            AppMemoryRead(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
                        }
                        AppMemoryRead(Key, KeyAddress, MFCLASSIC_MEM_KEY_SIZE);
                        /* Precalculate the reader response from card-nonce */
                        memcpy(ReaderResponse, CardNonceSuccessor1, MFCLASSIC_MEM_NONCE_SIZE);
                        /* Precalculate our response from the reader response */
                        memcpy(CardResponse, CardNonceSuccessor2, MFCLASSIC_MEM_NONCE_SIZE);
                        /* Setup crypto1 cipher. Discard in-place encrypted CardNonce. */
                        Crypto1Setup(Key, Uid, CardNonce, NULL);
                    }
                    /* Respond with the random card nonce and expect further authentication
                    * form the reader in the next frame. */
                    memcpy(Buffer, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
                    State = STATE_AUTHING;
                    retSize = MFCLASSIC_CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
                } else {
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                }
            } else if ( (Buffer[0] == MFCLASSIC_CMD_READ) ||
                        (Buffer[0] == MFCLASSIC_CMD_WRITE) ||
                        (Buffer[0] == MFCLASSIC_CMD_DECREMENT) ||
                        (Buffer[0] == MFCLASSIC_CMD_INCREMENT) ||
                        (Buffer[0] == MFCLASSIC_CMD_RESTORE) ||
                        (Buffer[0] == MFCLASSIC_CMD_TRANSFER) ) {
                State = STATE_IDLE;
                Buffer[0] = MFCLASSIC_NAK_TBKO_OPKO;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            } else {
                /* Unknown command. Enter HALT state. */
                State = STATE_HALT;
            }
            break; /* End of state ACTIVE */

        case STATE_AUTHING:
            if(isDetectionEnabled) {
                // Save reader's auth phase 2 answer to our nonce from STATE_ACTIVE
                memcpy(DetectionDataSave+DETECTION_SAVE_P2_OFFSET, Buffer, DETECTION_READER_AUTH_P2_SIZE);
                // Align data storage in each KEYX dedicated memory space, and iterate counters
                uint16_t memSaveAddr = (uint16_t)DETECTION_MEM_BLOCK0_SIZE;
                if (DetectionDataSave[DETECTION_KEYX_SAVE_IDX] == MFCLASSIC_CMD_AUTH_A) {
                    memSaveAddr += ((uint16_t)DetectionAttemptsKeyA * DETECTION_BYTES_PER_SAVE);
                    DetectionAttemptsKeyA++;
                    DetectionAttemptsKeyA = DetectionAttemptsKeyA % DETECTION_MEM_MAX_KEYX_SAVES;
                } else {
                    memSaveAddr += (DETECTION_MEM_KEYX_SEPARATOR_OFFSET + ((uint16_t)DetectionAttemptsKeyB * DETECTION_BYTES_PER_SAVE));
                    DetectionAttemptsKeyB++;
                    DetectionAttemptsKeyB = DetectionAttemptsKeyB % DETECTION_MEM_MAX_KEYX_SAVES;
                }
                // Write to app memory
                AppMemoryWrite(DetectionDataSave, memSaveAddr, DETECTION_BYTES_PER_SAVE);
                // Rage quit
                State = STATE_HALT;
            } else {
                /* Reader delivers an encrypted nonce. We use it
                * to setup the crypto1 LFSR in nonlinear feedback mode.
                * Furthermore it delivers an encrypted answer. Decrypt and check it */
                Crypto1Auth(&Buffer[0]);
                for (uint8_t i=0; i<4; i++) {
                    Buffer[i+4] ^= Crypto1Byte();
                }
                if ((Buffer[4] == ReaderResponse[0]) &&
                    (Buffer[5] == ReaderResponse[1]) &&
                    (Buffer[6] == ReaderResponse[2]) &&
                    (Buffer[7] == ReaderResponse[3])) {
                    /* Reader is authenticated. Encrypt the precalculated card response
                    * and generate the parity bits. */
                    for (uint8_t i=0; i<sizeof(CardResponse); i++) {
                        Buffer[i] = CardResponse[i] ^ Crypto1Byte();
                        Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = ODD_PARITY(CardResponse[i]) ^ Crypto1FilterOutput();
                    }
                    State = STATE_AUTHED_IDLE;
                    retSize = (MFCLASSIC_CMD_AUTH_BA_FRAME_SIZE * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
                } else {
                    /* Just reset on authentication error. */
                    State = STATE_HALT;
                }
            }
            break; /* End of state AUTHING */

        case STATE_AUTHED_IDLE:
            /* If something went wrong the reader might send an unencrypted halt */
            if (Buffer[0] == MFCLASSIC_CMD_HALT) {
                retSize = mfcHandleHaltCommand(Buffer);
            } else {
                /* In this state, all communication is encrypted. Thus we first have to decrypt
                * the incoming data. */
                for (uint8_t i=0; i<4; i++) {
                    Buffer[i] ^= Crypto1Byte();
                }
                /* All possible operations at this state have all same frame size, so we do
                 * CRC check first */
                if (!ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_COMMON_FRAME_SIZE)) {
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO ^ Crypto1Nibble();
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                /* If CRC is valid we continue, with CRC passed */
                } else {
                    if (Buffer[0] == MFCLASSIC_CMD_READ) {
                        /* Read command. Read data from memory and append CRCA. */
                        AppMemoryRead(Buffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                        ISO14443AAppendCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                        /* Encrypt and calculate parity bits. */
                        for (uint8_t i=0; i<(ISO14443A_CRCA_SIZE + MFCLASSIC_MEM_BYTES_PER_BLOCK); i++) {
                            uint8_t Plain = Buffer[i];
                            Buffer[i] = Plain ^ Crypto1Byte();
                            Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = ODD_PARITY(Plain) ^ Crypto1FilterOutput();
                        }
                        retSize = ( (MFCLASSIC_CMD_READ_RESPONSE_FRAME_SIZE + ISO14443A_CRCA_SIZE) * BITS_PER_BYTE )
                                  | ISO14443A_APP_CUSTOM_PARITY;
                    /* Write-type operation request */
                    } else if ( (Buffer[0] == MFCLASSIC_CMD_WRITE) || (Buffer[0] == MFCLASSIC_CMD_TRANSFER) ) {
                        /* Get target address. Write-type ops have address as 1st argument */
                        CurrentAddress = Buffer[1];
                        /* We deny any write-type operation to Block0 / Sector0 */
                        if (CurrentAddress == MFCLASSIC_MEM_S0B0_ADDRESS) {
                            Buffer[0] = MFCLASSIC_NAK_TBOK_OPKO ^ Crypto1Nibble();
                        } else if (Buffer[0] == MFCLASSIC_CMD_WRITE) {
                            /* Write command. Store the address and prepare for the upcoming data.
                            * Respond with ACK. */
                            State = STATE_WRITE;
                            Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                        } else if (Buffer[0] == MFCLASSIC_CMD_TRANSFER) {
                            /* Write back the global block buffer to the desired block address */
                            if (!ActiveConfiguration.ReadOnly) {
                                AppMemoryWrite(BlockBuffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                            } else {
                                /* In read only mode, silently ignore the write */
                            }
                            Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                        }
                        retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                    } else if (Buffer[0] == MFCLASSIC_CMD_DECREMENT) {
                        CurrentAddress = Buffer[1];
                        State = STATE_DECREMENT;
                        Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                        retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                    } else if (Buffer[0] == MFCLASSIC_CMD_INCREMENT) {
                        CurrentAddress = Buffer[1];
                        State = STATE_INCREMENT;
                        Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                        retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                    } else if (Buffer[0] == MFCLASSIC_CMD_RESTORE) {
                        CurrentAddress = Buffer[1];
                        State = STATE_RESTORE;
                        Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                        retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                    } else if ( (Buffer[0] == MFCLASSIC_CMD_AUTH_A) || (Buffer[0] == MFCLASSIC_CMD_AUTH_B) ) {
                        /* Nested authentication. */
                        uint8_t SectorAddress;
                        uint8_t KeyOffset = (Buffer[0] == MFCLASSIC_CMD_AUTH_A ? MFCLASSIC_MEM_KEY_A_OFFSET : MFCLASSIC_MEM_KEY_B_OFFSET);
                        /* Fix for MFClassic 4k cards */
                        if(Buffer[1] >= 128) {
                            SectorAddress = Buffer[1] & MFCLASSIC_MEM_BIGSECTOR_ADDR_MASK;
                            KeyOffset += MFCLASSIC_MEM_KEY_BIGSECTOR_OFFSET;
                        } else {
                            SectorAddress = Buffer[1] & MFCLASSIC_MEM_SECTOR_ADDR_MASK;
                        }
                        uint16_t KeyAddress = (uint16_t) SectorAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK + KeyOffset;
                        uint8_t Key[MFCLASSIC_MEM_KEY_SIZE];
                        uint8_t Uid[MFCLASSIC_UID_SIZE];
                        uint8_t CardNonce[MFCLASSIC_MEM_NONCE_SIZE] = {0x01};
                        uint8_t CardNonceParity[MFCLASSIC_MEM_NONCE_SIZE];
                        /* Generate a random nonce and read UID and key from memory */
                        //RandomGetBuffer(CardNonce, sizeof(CardNonce));
                        if (is7BytesUID) {
                            AppMemoryRead(Uid, MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
                        } else {
                            AppMemoryRead(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
                        }
                        AppMemoryRead(Key, KeyAddress, MFCLASSIC_MEM_KEY_SIZE);
                        /* Precalculate the reader response from card-nonce */
                        memcpy(ReaderResponse, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
                        Crypto1PRNG(ReaderResponse, 64);
                        /* Precalculate our response from the reader response */
                        memcpy(CardResponse, ReaderResponse, MFCLASSIC_MEM_NONCE_SIZE);
                        Crypto1PRNG(CardResponse, 32);
                        /* Setup crypto1 cipher for nested authentication. */
                        Crypto1Setup(Key, Uid, CardNonce, CardNonceParity);
                        for (uint8_t i=0; i<sizeof(CardNonce); i++) {
                            Buffer[i] = CardNonce[i];
                            Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = CardNonceParity[i];
                        }
                        /* Respond with the encrypted random card nonce and expect further authentication
                        * form the reader in the next frame. */
                        State = STATE_AUTHING;
                        retSize = (MFCLASSIC_CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
                    } else {
                        /* Unknown command. Enter HALT state */
                        State = STATE_HALT;
                    }
                } /* End of if/else CRC check condition */
            }
            break; /* End of state AUTHED_IDLE */

        case STATE_WRITE:
            /* The reader has issued a write command earlier and is now
             * sending the data to be written. Decrypt the data first and
             * check for CRC. Then write the data when ReadOnly mode is not
             * activated. */
            /* We receive 16 bytes of data to be written and 2 bytes CRCA. Decrypt */
            for (uint8_t i=0; i<(MFCLASSIC_MEM_BYTES_PER_BLOCK + ISO14443A_CRCA_SIZE); i++) {
                Buffer[i] ^= Crypto1Byte();
            }
            if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK)) {
                if (!ActiveConfiguration.ReadOnly) {
                    AppMemoryWrite(Buffer, CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                } else {
                    /* Silently ignore in ReadOnly mode */
                }
                Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO ^ Crypto1Nibble();
            }
            State = STATE_AUTHED_IDLE;
            retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            break; /* End of state WRITE */

        case STATE_DECREMENT:
        case STATE_INCREMENT:
        case STATE_RESTORE:
            /* When we reach here, a decrement, increment or restore command has
             * been issued earlier and the reader is now sending the data. First,
             * decrypt the data and check CRC. Read data from the requested block
             * address into the global block buffer and check for integrity. Then
             * add or subtract according to issued command if necessary and store
             * the block back into the global block buffer. */
            for (uint8_t i=0; i<(MFCLASSIC_MEM_VALUE_SIZE  + ISO14443A_CRCA_SIZE); i++) {
                Buffer[i] ^= Crypto1Byte();
            }
            if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_VALUE_SIZE )) {
                AppMemoryRead(BlockBuffer, (uint16_t) CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                if (CheckValueIntegrity(BlockBuffer)) {
                    uint32_t ParamValue;
                    uint32_t BlockValue;
                    ValueFromBlock(&ParamValue, Buffer);
                    ValueFromBlock(&BlockValue, BlockBuffer);
                    if (State == STATE_DECREMENT) {
                        BlockValue -= ParamValue;
                    } else if (State == STATE_INCREMENT) {
                        BlockValue += ParamValue;
                    } else if (State == STATE_RESTORE) {
                        /* Do nothing as all is already done in AUTHED_IDLE */
                    }
                    ValueToBlock(BlockBuffer, BlockValue);
                    State = STATE_AUTHED_IDLE;
                    /* No ACK response on value commands part 2 */
                    retSize = ISO14443A_APP_NO_RESPONSE;
                } else {
                    Buffer[0] = MFCLASSIC_NAK_TBKO_CRCKO ^ Crypto1Nibble();
                }
            } else {
                /* CRC Error. */
                Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO ^ Crypto1Nibble();
            }
            State = STATE_AUTHED_IDLE;
            retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            break; /* End of states DECREMENT, INCREMENT and RESTORE */

        default:
            /* Unknown state should never happen */
            retSize = ISO14443A_APP_NO_RESPONSE;
        } /* End of states switch/case */
    } /* End of if/else WUPA/REQA condition */

    return retSize;
}

void MifareClassicGetUid(ConfigurationUidType Uid) {
    if (is7BytesUID) {
        AppMemoryRead(&Uid[0], MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE-1);
        AppMemoryRead(&Uid[3], MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
    } else {
        AppMemoryRead(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
    }
}

void MifareClassicSetUid(ConfigurationUidType Uid) {
    if (is7BytesUID) {
        AppMemoryWrite(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, ActiveConfiguration.UidSize);
    } else {
        uint8_t BCC =  Uid[0] ^ Uid[1] ^ Uid[2] ^ Uid[3];
        AppMemoryWrite(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
        AppMemoryWrite(&BCC, MFCLASSIC_MEM_UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
    }
}

void MifareClassicGetAtqa(uint16_t * Atqa) {
    uint8_t addrATQA = (is7BytesUID) ? (MFCLASSIC_MEM_7B_ATQA_ADDR) : (MFCLASSIC_MEM_ATQA_ADDR);
    AppMemoryRead(((uint8_t *)&CardATQAValue), addrATQA, MFCLASSIC_MEM_ATQA_SIZE-1);
    AppMemoryRead(((uint8_t *)&CardATQAValue)+1, addrATQA+1, MFCLASSIC_MEM_ATQA_SIZE-1);
    *Atqa = CardATQAValue;
}

void MifareClassicSetAtqa(uint16_t Atqa) {
    CardATQAValue = Atqa;
    uint8_t addrATQA = (is7BytesUID) ? (MFCLASSIC_MEM_7B_ATQA_ADDR) : (MFCLASSIC_MEM_ATQA_ADDR);
    AppMemoryWrite(((uint8_t *)&CardATQAValue), addrATQA, MFCLASSIC_MEM_ATQA_SIZE-1);
    AppMemoryWrite(((uint8_t *)&CardATQAValue)+1, addrATQA+1, MFCLASSIC_MEM_ATQA_SIZE-1);
}

void MifareClassicGetSak(uint8_t * Sak) {
    uint8_t addrSAK = (is7BytesUID) ? (MFCLASSIC_MEM_7B_SAK_ADDR) : (MFCLASSIC_MEM_SAK_ADDR);
    AppMemoryRead(&CardSAKValue, addrSAK, MFCLASSIC_MEM_SAK_SIZE);
    *Sak = CardSAKValue;
}

void MifareClassicSetSak(uint8_t Sak) {
    CardSAKValue = Sak;
    uint8_t addrSAK = (is7BytesUID) ? (MFCLASSIC_MEM_7B_SAK_ADDR) : (MFCLASSIC_MEM_SAK_ADDR);
    AppMemoryWrite(&CardSAKValue, addrSAK, MFCLASSIC_MEM_SAK_SIZE);
}
