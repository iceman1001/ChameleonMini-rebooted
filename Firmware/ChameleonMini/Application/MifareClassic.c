/*
 * MifareClassic.c
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#if defined(CONFIG_MF_CLASSIC_SUPPORT) || defined(CONFIG_MF_CLASSIC_DETECTION_SUPPORT) \
    || defined(CONFIG_MF_CLASSIC_BRUTE_SUPPORT) || defined(SUPPORT_MF_CLASSIC_MAGIC_MODE) \
    || defined(CONFIG_MF_CLASSIC_LOG_SUPPORT)

#include "MifareClassic.h"
#include "ISO14443-3A.h"
#include "Crypto1.h"
#include "../Random.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory/Memory.h"
#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
#include "System.h"
#endif

// UNUSED
// /* Decoding table for Access conditions of a data block */
// static const uint8_t abBlockAccessConditions[8][2] = {
//     /*C1C2C3 */
//     /* 0 0 0 R:key A|B W: key A|B I:key A|B D:key A|B     transport configuration */
//     {
//         /* Access with Key A */
//         MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT
//     },
//     /* 1 0 0 R:key A|B W:key B I:never D:never     read/write block */
//     {
//         /* Access with Key A */
//         MFCLASSIC_ACC_BLOCK_READ,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE
//     },
//     /* 0 1 0 R:key A|B W:never I:never D:never     read/write block */
//     {
//         /* Access with Key A */
//         MFCLASSIC_ACC_BLOCK_READ,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ
//     },
//     /* 1 1 0 R:key A|B W:key B I:key B D:key A|B     value block */
//     {
//         /* Access with Key A */
//         MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE | MFCLASSIC_ACC_BLOCK_INCREMENT | MFCLASSIC_ACC_BLOCK_DECREMENT
//     },
//     /* 0 0 1 R:key A|B W:never I:never D:key A|B     value block */
//     {
//         /* Access with Key A */
//         MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ  |  MFCLASSIC_ACC_BLOCK_DECREMENT
//     },
//     /* 1 0 1 R:key B W:never I:never D:never     read/write block */
//     {
//         /* Access with Key A */
//         0,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ
//     },
//     /* 0 1 1 R:key B W:key B I:never D:never    read/write block */
//     {
//         /* Access with Key A */
//         0,
//         /* Access with Key B */
//         MFCLASSIC_ACC_BLOCK_READ | MFCLASSIC_ACC_BLOCK_WRITE
//     },
//     /* 1 1 1 R:never W:never I:never D:never    read/write block */
//     {
//         /* Access with Key A */
//         0,
//         /* Access with Key B */
//         0
//     }
// 
// };
/* Decoding table for Access conditions of the sector trailor */
static const uint8_t abTrailorAccessConditions[8][2] = {
    /* 0  0  0 RdKA:never WrKA:key A  RdAcc:key A WrAcc:never  RdKB:key A WrKB:key A      Key B may be read[1] */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB,
        /* Access with Key B */
        0
    },
    /* 1  0  0 RdKA:never WrKA:key B  RdAcc:keyA|B WrAcc:never RdKB:never WrKB:key B */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        /* Access with Key B */
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC |  MFCLASSIC_ACC_TRAILOR_WRITE_KEYB
    },
    /* 0  1  0 RdKA:never WrKA:never  RdAcc:key A WrAcc:never  RdKB:key A WrKB:never  Key B may be read[1] */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB,
        /* Access with Key B */
        0
    },
    /* 1  1  0         never never  keyA|B never never never */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        /* Access with Key B */
        MFCLASSIC_ACC_TRAILOR_READ_ACC
    },
    /* 0  0  1         never key A  key A  key A key A key A  Key B may be read,transport configuration[1] */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_READ_KEYB | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB,
        /* Access with Key B */
        0
    },
    /* 0  1  1         never key B  keyA|B key B never key B */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        /* Access with Key B */
        MFCLASSIC_ACC_TRAILOR_WRITE_KEYA | MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_KEYB
    },
    /* 1  0  1         never never  keyA|B key B never never */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        /* Access with Key B */
        MFCLASSIC_ACC_TRAILOR_READ_ACC | MFCLASSIC_ACC_TRAILOR_WRITE_ACC
    },
    /* 1  1  1         never never  keyA|B never never never */
    {
        /* Access with Key A */
        MFCLASSIC_ACC_TRAILOR_READ_ACC,
        /* Access with Key B */
        MFCLASSIC_ACC_TRAILOR_READ_ACC
    },
};

enum estate {
    STATE_HALT,
    STATE_IDLE,
    STATE_CHINESE_IDLE,
    STATE_CHINESE_WRITE,
    STATE_READY,
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

#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
char *estate_str[] = {
    "HALT",
    "IDLE",
    "CHINESE_IDLE",
    "CHINESE_WRITE",
    "READY",
    "ACTIVE",
    "AUTHING",
    "AUTHED_IDLE",
    "WRITE",
    "INCREMENT",
    "DECREMENT",
    "RESTORE"
};
#endif

static uint8_t CardResponse[MFCLASSIC_MEM_NONCE_SIZE];
static uint8_t ReaderResponse[MFCLASSIC_MEM_NONCE_SIZE];
static uint8_t CurrentAddress;
static uint8_t KeyInUse;
static uint8_t BlockBuffer[MFCLASSIC_MEM_BYTES_PER_BLOCK];
static uint8_t AccessConditions[MFCLASSIC_MEM_ACC_GPB_SIZE]; // Access Conditions + General purpose Byte
static uint8_t AccessAddress;
static uint16_t CardATQAValue;
static uint8_t CardSAKValue;
static bool is7BytesUID = false;
/* To differentiate between IDLE and HALT starts */
static bool isFromHaltChain = false;
/* To check if previous step of any cascading sequence has passed */
static bool isCascadeStepOnePassed = false;
/* To enable MF_CLASSIC_DETECTION behavior */
static bool isDetectionEnabled = false;
#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
static bool isDetectionCanaryWritten = false;
static uint8_t DetectionCanary[DETECTION_BLOCK0_CANARY_SIZE] = { DETECTION_BLOCK0_CANARY };
static uint8_t DetectionDataSave[DETECTION_BYTES_PER_SAVE] = {0};
static uint8_t DetectionAttemptsKeyA = 0;
static uint8_t DetectionAttemptsKeyB = 0;
#endif
#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
static bool isBruteEnabled = false;
static uint8_t BruteIdleRounds = 0;
static uint32_t BruteCurrentUid = 0;
#endif
#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
static bool isLogEnabled = false;
static uint32_t LogBytesWrote = 0;
static uint16_t LogBytesBuffered = 0;
static uint32_t LogMaxBytes = 0;
static uint8_t LogLineBufferA[MFCLASSIC_LOG_MEM_LINE_BUFFER_LEN] = { 0 };
static uint8_t LogLineBufferB[MFCLASSIC_LOG_MEM_LINE_BUFFER_LEN] = { 0 };
static uint8_t * LogLineBuffer = LogLineBufferA;
static bool LogLineBufferFirst = true;
#endif

/* decode Access conditions for a block */
INLINE uint8_t GetAccessCondition(uint8_t Block) {
    uint8_t  InvSAcc0;
    uint8_t  InvSAcc1;
    uint8_t  Acc0 = AccessConditions[0];
    uint8_t  Acc1 = AccessConditions[1];
    uint8_t  Acc2 = AccessConditions[2];
    uint8_t  ResultForBlock = 0;

    InvSAcc0 = ~MFCLASSIC_BYTE_SWAP(Acc0);
    InvSAcc1 = ~MFCLASSIC_BYTE_SWAP(Acc1);

    /* Check */
    if (((InvSAcc0 ^ Acc1) & 0xf0) ||    /* C1x */
            ((InvSAcc0 ^ Acc2) & 0x0f) ||   /* C2x */
            ((InvSAcc1 ^ Acc2) & 0xf0)) {   /* C3x */
        return (MFCLASSIC_ACC_NO_ACCESS);
    }
    /* Fix for MFClassic 4K cards */
    if (Block < 128)
        Block &= 3;
    else {
        Block &= 15;
        if (Block & 15)
            Block = 3;
        else if (Block <= 4)
            Block = 0;
        else if (Block <= 9)
            Block = 1;
        else
            Block = 2;
    }

    Acc0 = ~Acc0;       /* C1x Bits to bit 0..3 */
    Acc1 =  Acc2;       /* C2x Bits to bit 0..3 */
    Acc2 =  Acc2 >> 4;  /* C3x Bits to bit 0..3 */

    if (Block) {
        Acc0 >>= Block;
        Acc1 >>= Block;
        Acc2 >>= Block;
    }
    /* combine the bits */
    ResultForBlock = ((Acc2 & 1) << 2) |
                     ((Acc1 & 1) << 1) |
                     (Acc0 & 1);
    return (ResultForBlock);
}

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

#if defined(CONFIG_MF_CLASSIC_BRUTE_SUPPORT) || defined(CONFIG_MF_CLASSIC_LOG_SUPPORT)
uint32_t BytesToUint32(uint8_t * Buffer) {
    return ( (((uint32_t)(Buffer[0])) << 24)
            | (((uint32_t)(Buffer[1])) << 16)
            | (((uint32_t)(Buffer[2])) << 8)
            | ((uint32_t)(Buffer[3])) );
}

uint16_t BytesToUint16(uint8_t * Buffer) {
    return ( (((uint16_t)(Buffer[0])) << 8) | ((uint16_t)(Buffer[1])) );
}

void Uint32ToBytes(uint32_t Uint32, uint8_t * Buffer) {
    Buffer[0] = ((uint8_t)(Uint32 >> 24) & 0xFF);
    Buffer[1] = ((uint8_t)(Uint32 >> 16) & 0xFF);
    Buffer[2] = ((uint8_t)(Uint32 >> 8) & 0xFF);
    Buffer[3] = ((uint8_t)(Uint32 & 0xFF));
}

void Uint16ToBytes(uint16_t Uint16, uint8_t * Buffer) {
    Buffer[0] = ((uint8_t)(Uint16 >> 8) & 0xFF);
    Buffer[1] = ((uint8_t)(Uint16 & 0xFF));
}
#endif

void MifareClassicAppInit(uint16_t ATQA_4B, uint8_t SAK, bool is7B) {
    State = STATE_IDLE;
    is7BytesUID = is7B;
    CardATQAValue = (is7BytesUID) ? (ATQA_4B | MFCLASSIC_7B_ATQA_MASK) : (ATQA_4B);
    CardSAKValue = SAK;
    isFromHaltChain = false;
    isCascadeStepOnePassed = false;
}

void MifareClassicAppInit1K(void) {
    MifareClassicAppInit( MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE,
                          (ActiveConfiguration.UidSize == MFCLASSIC_UID_7B_SIZE) );
}

void MifareClassicAppInit4K(void) {
    MifareClassicAppInit( MFCLASSIC_4K_ATQA_VALUE, MFCLASSIC_4K_SAK_VALUE,
                          (ActiveConfiguration.UidSize == MFCLASSIC_UID_7B_SIZE) );
}

void MifareClassicAppInitMini(void) {
    MifareClassicAppInit(MFCLASSIC_MINI_ATQA_VALUE, MFCLASSIC_MINI_SAK_VALUE, false);
}

#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
void MifareClassicAppDetectionInit(void) {
    isDetectionEnabled = true;
    DetectionAttemptsKeyA = 0;
    DetectionAttemptsKeyB = 0;
    MifareClassicAppInit(MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE, false);
}
#endif

#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
void MifareClassicAppBruteGetCurrentUid(void) {
    uint8_t TempUid[MFCLASSIC_UID_SIZE];
    MifareClassicGetUid(TempUid);
    BruteCurrentUid = BytesToUint32(TempUid);
}

void MifareClassicAppBruteWrite(void) {
    uint8_t bruteStatusByte = (isBruteEnabled) ? (BRUTE_MEM_BRUTED_STATUS_CANARY) : (BRUTE_MEM_BRUTED_STATUS_RESET);
    AppWorkingMemoryWrite(&bruteStatusByte, BRUTE_MEM_BRUTED_STATUS_ADDR, BRUTE_MEM_BRUTED_STATUS_SIZE);
    uint8_t TempUid[MFCLASSIC_UID_SIZE];
    Uint32ToBytes(BruteCurrentUid, TempUid);
    MifareClassicSetUid(TempUid);
}

void MifareClassicAppBruteInit(void) {
    MifareClassicAppInit(MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE, false);
    uint8_t bruteStatusByte = BRUTE_MEM_BRUTED_STATUS_CANARY;
    AppWorkingMemoryRead(&bruteStatusByte, BRUTE_MEM_BRUTED_STATUS_ADDR, BRUTE_MEM_BRUTED_STATUS_SIZE);
    isBruteEnabled = (bruteStatusByte == BRUTE_MEM_BRUTED_STATUS_CANARY);
    BruteIdleRounds = 0;
    MifareClassicAppBruteGetCurrentUid();
}

void MifareClassicAppBruteStop(void) {
    isBruteEnabled = false;
    BruteIdleRounds = 0;
    MifareClassicAppBruteWrite();
}

void MifareClassicAppBruteMove(void) {
    isBruteEnabled = true;
    BruteIdleRounds = 0;
    BruteCurrentUid++;
    State = STATE_IDLE;
    MifareClassicAppBruteWrite();
}

void MifareClassicAppBruteToggle(void) {
    if(isBruteEnabled) {
        MifareClassicAppBruteStop();
    } else {
        MifareClassicAppBruteGetCurrentUid();
        MifareClassicAppBruteMove();
    }
}

void MifareClassicAppBruteTick(void) {
    // If we were using same UID for too long, change it
    if( isBruteEnabled ) {
        if( BruteIdleRounds >= BRUTE_IDLE_MAX_ROUNDS ) {
            MifareClassicAppBruteMove();
        } else {
            BruteIdleRounds++;
        }
    }
}
#endif

#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
void MifareClassicAppLogCheck(void) {
    uint8_t headerLine[MFCLASSIC_LOG_MEM_LOG_HEADER_LEN];
    AppWorkingMemoryRead(&headerLine, MFCLASSIC_LOG_MEM_LOG_HEADER_ADDR, MFCLASSIC_LOG_MEM_LOG_HEADER_LEN);
    if( (headerLine[MFCLASSIC_LOG_MEM_STATUS_CANARY_ADDR] == MFCLASSIC_LOG_MEM_STATUS_CANARY)
        || (headerLine[MFCLASSIC_LOG_MEM_STATUS_CANARY_ADDR] == MFCLASSIC_LOG_MEM_STATUS_RESET) ) {
        isLogEnabled = (headerLine[MFCLASSIC_LOG_MEM_STATUS_CANARY_ADDR] == MFCLASSIC_LOG_MEM_STATUS_CANARY);
        LogBytesWrote = BytesToUint32(&headerLine[MFCLASSIC_LOG_MEM_WROTEBYTES_ADDR]);
    } else {
        isLogEnabled = true;
        LogBytesWrote = 0;
    }
}

void MifareClassicAppLogWriteHeader(void) {
    uint8_t headerLine[MFCLASSIC_LOG_MEM_LOG_HEADER_LEN] = { 0 };
    headerLine[MFCLASSIC_LOG_MEM_STATUS_CANARY_ADDR] = (isLogEnabled) ? (MFCLASSIC_LOG_MEM_STATUS_CANARY) : (MFCLASSIC_LOG_MEM_STATUS_RESET);
    Uint32ToBytes(LogBytesWrote, &headerLine[MFCLASSIC_LOG_MEM_WROTEBYTES_ADDR]);
    AppWorkingMemoryWrite(headerLine, MFCLASSIC_LOG_MEM_LOG_HEADER_ADDR, MFCLASSIC_LOG_MEM_LOG_HEADER_LEN);
}

void MifareClassicAppLogBufferLine(const uint8_t * Data, uint16_t BitCount, uint8_t Source) {
    uint16_t dataBytesToBuffer = (BitCount / BITS_PER_BYTE);
    if(BitCount % BITS_PER_BYTE) dataBytesToBuffer++;

    uint16_t logStateStrLen = strlen(estate_str[State]);
    uint16_t idx = LogBytesBuffered+MFCLASSIC_LOG_MEM_LINE_START_ADDR;

    if( (idx + dataBytesToBuffer*2 + logStateStrLen + 14) < MFCLASSIC_LOG_MEM_LINE_BUFFER_LEN) {
        idx += sprintf((char *)&LogLineBuffer[idx],"[%05u] %c:\t%s\t| ",SystemGetSysTick(),Source,estate_str[State]);
	BufferToHexString((char *)&LogLineBuffer[idx],dataBytesToBuffer*2+1,Data,dataBytesToBuffer);
        idx += dataBytesToBuffer*2;
        idx += sprintf((char *)&LogLineBuffer[idx]," ;");
    }
    if(MFCLASSIC_LOG_MEM_LINE_BUFFER_LEN - idx > 2){
        idx += sprintf((char *)&LogLineBuffer[idx],"\r\n");
    }
	LogBytesBuffered = idx;
}

void MifareClassicAppLogWriteLines(void) {
    uint16_t LogBytesToWrite = LogBytesBuffered;
    uint8_t * LogLineBufferToWrite = LogLineBuffer;
    /* swap buffers */
    if ( LogLineBufferFirst ) {
        LogLineBuffer = LogLineBufferB;
	LogLineBufferFirst = false;
    } else {
        LogLineBuffer = LogLineBufferA;
	LogLineBufferFirst = true;
    }
    LogBytesBuffered = 0;

    if( isLogEnabled && (LogBytesToWrite > 0) ) {
	/* circular log */
        if( (LogBytesWrote + LogBytesToWrite) >= LogMaxBytes) {
            LogBytesWrote = 0;
        }
        /* write log */
        AppWorkingMemoryWrite(LogLineBufferToWrite, MFCLASSIC_LOG_MEM_LOG_HEADER_LEN+LogBytesWrote, LogBytesToWrite);
	/* update header */
        LogBytesWrote += LogBytesToWrite;
        MifareClassicAppLogWriteHeader();
    }
}

void MifareClassicAppLogStop(void) {
    isLogEnabled = false;
    MifareClassicAppLogWriteHeader();
}

void MifareClassicAppLogStart(void) {
    isLogEnabled = true;
    MifareClassicAppLogWriteHeader();
}

void MifareClassicAppLogInit(void) {
    MifareClassicAppInit(MFCLASSIC_1K_ATQA_VALUE, MFCLASSIC_1K_SAK_VALUE, false);
    MifareClassicAppLogCheck();
    LogMaxBytes = ( AppWorkingMemorySize() - MFCLASSIC_LOG_MEM_LOG_HEADER_LEN );
    LogBytesBuffered = 0;
}

void MifareClassicAppLogToggle(void) {
    if(isLogEnabled) {
        MifareClassicAppLogStop();
    } else {
        MifareClassicAppLogStart();
    }
}
#endif

void MifareClassicAppReset(void) {
    State = STATE_IDLE;
}

/* Handle a MFCLASSIC_CMD_HALT during main process, as can be raised in many states.
* Sets State, response buffer and response size. Returns if valid HALT. */
bool mfcHandleHaltCommand(uint8_t * Buffer, uint16_t * RetValue) {
    bool ret = false;
    /* Halts the tag. According to the ISO 14443-3, the second byte is supposed to be 0 */
    if ( (Buffer[0] == MFCLASSIC_CMD_HALT) && (Buffer[1] == 0) ) {
        /* If we get a buggy HALT, we fallback to IDLE or HALT depending on origin */
        State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
        ret = true;
        /* If valid HALT with CRC passed */
        if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_HALT_FRAME_SIZE)) {
            /* According to ISO 14443-3, we must not send anything to ACK */
            State = STATE_HALT;
            *RetValue = ISO14443A_APP_NO_RESPONSE;
        } else {
            Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
            *RetValue = MFCLASSIC_ACK_NAK_FRAME_SIZE;
        }
    }
    return ret;
}

/* Handle a WUPA or REQA during main process, as can be raised in all states.
* Sets State, response buffer and response size. Returns if WUPA/REQA received. */
bool mfcHandleWUPCommand(bool isFromHaltState, uint8_t * Buffer, uint16_t BitCount, uint16_t ATQAValue, uint16_t * RetValue) {
    bool ret = false;
    /* If we have been awoken */
    if ( (BitCount == MFCLASSIC_CMD_WUPA_BITCOUNT) && ISO14443AIsWakeUp(Buffer, isFromHaltState) ) {
        /* Set response buffer */
        ISO14443ASetWakeUpResponse(Buffer, ATQAValue);
        ret = true;
        /* If valid WUPA or REQA, go to READY state */
        if ( (State == STATE_IDLE) || (State == STATE_HALT) ) {
            AccessAddress = MFCLASSIC_MEM_INVALID_ADDRESS;
            State = STATE_READY;
            *RetValue = ISO14443A_ATQA_FRAME_SIZE;
        /* Else we go back to IDLE or HALT, depending on where
         * we come from, as per ISO 14443-3 */
        } else {
            State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
            *RetValue = ISO14443A_APP_NO_RESPONSE;
        }
    }
    return ret;
}

/* Handle an authentication request.
* Sets State, response buffer and response size. */
void mfcHandleAuthenticationRequest(bool isNested, uint8_t * Buffer, uint16_t * RetValue) {
    uint16_t SectorAddress = MFCLASSIC_MEM_INVALID_ADDRESS;
    uint8_t Key[MFCLASSIC_MEM_KEY_SIZE];
    uint8_t Uid[MFCLASSIC_UID_SIZE];
    uint16_t KeyOffset = (Buffer[0] == MFCLASSIC_CMD_AUTH_A) ? MFCLASSIC_MEM_KEY_A_OFFSET : MFCLASSIC_MEM_KEY_B_OFFSET;
    uint16_t AccessOffset = MFCLASSIC_MEM_KEY_A_OFFSET + MFCLASSIC_MEM_KEY_SIZE;
    uint16_t KeyAddress;
    //uint8_t Sector = Buffer[1];

    /* Save Nonce in detection mode */
    if(isDetectionEnabled && !isNested) {
#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
        memset(DetectionDataSave, 0x00, DETECTION_BYTES_PER_SAVE);
        // Save reader's auth phase 1: KEY type (A or B), and sector number
        memcpy(DetectionDataSave, Buffer, DETECTION_READER_AUTH_P1_SIZE);
        // Set selected key to be the DETECTION canary
        KeyAddress = DETECTION_BLOCK0_CANARY_ADDR;
#endif
    /* Set key address and loads it */
    } else {
        /* Fix for MFClassic 4k cards */
        if(Buffer[1] >= 128) {
            SectorAddress = (Buffer[1] & MFCLASSIC_MEM_BIGSECTOR_ADDR_MASK) * MFCLASSIC_MEM_BYTES_PER_BLOCK;
            KeyOffset += MFCLASSIC_MEM_KEY_BIGSECTOR_OFFSET;
            AccessOffset += MFCLASSIC_MEM_KEY_BIGSECTOR_OFFSET;
        } else {
            SectorAddress = (Buffer[1] & MFCLASSIC_MEM_SECTOR_ADDR_MASK) * MFCLASSIC_MEM_BYTES_PER_BLOCK;
        }
        KeyAddress = (uint16_t) SectorAddress + KeyOffset;
    }

    /* set KeyInUse for global use to keep info about authentication */
    KeyInUse = Buffer[0] & 1;
    CurrentAddress = SectorAddress / MFCLASSIC_MEM_BYTES_PER_BLOCK;
    //if (!AccessConditions[MEM_ACC_GPB_SIZE-1] ||(CurrentAddress != AccessAddress)) {
    /* Get access conditions from the sector trailor */
    AppCardMemoryRead(AccessConditions, SectorAddress + AccessOffset, MFCLASSIC_MEM_ACC_GPB_SIZE);
    AccessAddress = CurrentAddress;
    //}

    /* Read UID and key from memory */
    if (is7BytesUID) {
        AppCardMemoryRead(Uid, MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
    } else {
        AppCardMemoryRead(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
    }
    AppCardMemoryRead(Key, KeyAddress, MFCLASSIC_MEM_KEY_SIZE);

    /* Proceed with nested or regular authent */
    if(isNested) {
        uint8_t CardNonce[MFCLASSIC_MEM_NONCE_SIZE] = {0x01};
        uint8_t CardNonceParity[MFCLASSIC_MEM_NONCE_SIZE];
         /* Precalculate the reader response from card-nonce */
        memcpy(ReaderResponse, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
        Crypto1PRNG(ReaderResponse, 64);
        /* Precalculate our response from the reader response */
        memcpy(CardResponse, ReaderResponse, MFCLASSIC_MEM_NONCE_SIZE);
        Crypto1PRNG(CardResponse, 32);
        /* Setup crypto1 cipher for nested authentication. */
        Crypto1Setup(Key, Uid, CardNonce, CardNonceParity);
        for (uint8_t i=0; i<MFCLASSIC_MEM_NONCE_SIZE; i++) {
            Buffer[i] = CardNonce[i];
            Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = CardNonceParity[i];
        }
        *RetValue = (MFCLASSIC_CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
    } else {
        uint8_t CardNonce[MFCLASSIC_MEM_NONCE_SIZE] = {0x01, 0x20, 0x01, 0x45};
        uint8_t CardNonceSuccessor1[MFCLASSIC_MEM_NONCE_SIZE] = {0x63, 0xe5, 0xbc, 0xa7};
        uint8_t CardNonceSuccessor2[MFCLASSIC_MEM_NONCE_SIZE] = {0x99, 0x37, 0x30, 0xbd};
#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
        if(isDetectionEnabled) {
            // Save sent 'random' nonce
            memcpy(DetectionDataSave+DETECTION_READER_AUTH_P1_SIZE, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
        }
#endif
        /* Precalculate the reader response from card-nonce */
        memcpy(ReaderResponse, CardNonceSuccessor1, MFCLASSIC_MEM_NONCE_SIZE);
        /* Precalculate our response from the reader response */
        memcpy(CardResponse, CardNonceSuccessor2, MFCLASSIC_MEM_NONCE_SIZE);
        /* Respond with the random card nonce and expect further authentication
        * form the reader in the next frame. */
        memcpy(Buffer, CardNonce, MFCLASSIC_MEM_NONCE_SIZE);
        /* Setup crypto1 cipher. Discard in-place encrypted CardNonce. */
        Crypto1Setup(Key, Uid, CardNonce, NULL);
        *RetValue = MFCLASSIC_CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
    }
    State = STATE_AUTHING;
}

/* Decrypt an encrypted buffer */
void mfcDecryptBuffer(uint8_t * Buffer, uint8_t Size) {
    for (uint8_t i=0; i < Size; i++) {
        Buffer[i] ^= Crypto1Byte();
    }
}

/* Encrypt and calculate parity bits for a response buffer */
void mfcEncryptBuffer(uint8_t * Output, uint8_t * Input, uint8_t Size) {
    for (uint8_t i=0; i < Size; i++) {
        uint8_t Plain = Input[i];
        Output[i] = Plain ^ Crypto1Byte();
        Output[ISO14443A_BUFFER_PARITY_OFFSET + i] = ODD_PARITY(Plain) ^ Crypto1FilterOutput();
    }
}

uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount) {
#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
    /* Log what comes from reader if logging enabled */
    if(isLogEnabled) {
        MifareClassicAppLogBufferLine(Buffer, BitCount, MFCLASSIC_LOG_READER);
    }
#endif
    /* Size of data (byte) we will send back to reader. Is main process return value */
    uint16_t retSize = ISO14443A_APP_NO_RESPONSE;
    /* WUPA/REQA and HALT may occur in every state. We handle is first, so we can skip
    * states cases if we get valid WUPA/REQA */
    if( (!mfcHandleWUPCommand((State == STATE_HALT), Buffer, BitCount, CardATQAValue, &retSize))
        && (!mfcHandleHaltCommand(Buffer, &retSize)) ) {
        switch(State) {
        case STATE_IDLE:
        case STATE_HALT:
            isFromHaltChain = (State == STATE_HALT);
            isCascadeStepOnePassed = false;
#ifdef SUPPORT_MF_CLASSIC_MAGIC_MODE
            if (Buffer[0] == MFCLASSIC_CMD_CHINESE_UNLOCK && AppMemoryUidMode()) {
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
                    AppCardMemoryRead(Buffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
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
               mfcHandleHaltCommand(Buffer, &retSize);
            }
            break; /* End of state CHINESE_IDLE */

        case STATE_CHINESE_WRITE:
            if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK)) {
                /* CRC check passed. Write data into memory and send ACK. */
                if (!ActiveConfiguration.ReadOnly) {
                    AppCardMemoryWrite(Buffer, CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
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

        case STATE_READY:
            /* Anticol/selection */
            if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
                uint8_t UidCL[ISO14443A_CL_UID_SIZE];
                uint8_t * UidCLReadBuffer;
                uint8_t UidMemAddr, UidReadSize, SAK;
                enum estate NextState;
                UidMemAddr = MFCLASSIC_MEM_UID_CL1_ADDRESS;
                /* First step of anticol/selection as per MF1S50YYX_V1, title 10.1.2 */
                if (is7BytesUID) {
                    UidCL[0] = ISO14443A_UID0_CT;
                    UidCLReadBuffer = &UidCL[1];
                    UidReadSize = MFCLASSIC_MEM_UID_CL1_SIZE-1;
                    SAK = MFCLASSIC_SAK_CL1_VALUE;
                    NextState = STATE_READY;
                /* 'Sequence 3' (no next step) as per MF1S50YYX_V1, title 10.1.2 */
                } else {
                    UidCLReadBuffer = UidCL;
                    UidReadSize = MFCLASSIC_MEM_UID_CL1_SIZE;
                    SAK = CardSAKValue;
                    NextState = STATE_ACTIVE;
                }
                AppCardMemoryRead(UidCLReadBuffer, UidMemAddr, UidReadSize);
                if (ISO14443ASelect(Buffer, &BitCount, UidCL, SAK)) {
                    AccessAddress = MFCLASSIC_MEM_INVALID_ADDRESS;
                    State = NextState;
                    isCascadeStepOnePassed = true;
                }
                /* Will be frame size if selected, or 0 else, as set by ISO14443ASelect */
                retSize = BitCount;
            /* Second cascade step of anticol/selection as per MF1S50YYX_V1, title 10.1.2 */
            } else if (isCascadeStepOnePassed) {
                /* 'Sequence 1' as per MF1S50YYX_V1, title 10.1.2 */
                if ( is7BytesUID && (Buffer[0] == ISO14443A_CMD_SELECT_CL2) ) {
                    uint8_t UidCL[ISO14443A_CL_UID_SIZE];
                    AppCardMemoryRead(UidCL, MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
                    if (ISO14443ASelect(Buffer, &BitCount, UidCL, CardSAKValue)) {
                        State = STATE_ACTIVE;
                        isCascadeStepOnePassed = false;
                    }
                    retSize = BitCount;
                /* 'Sequence 2' as per MF1S50YYX_V1, title 10.1.2 */
                } else if (Buffer[0] == MFCLASSIC_CMD_READ) {
                    /* Read sector 0 / block 0 and send in plain */
                    AppCardMemoryRead(Buffer, MFCLASSIC_MEM_S0B0_ADDRESS, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                    ISO14443AAppendCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                    State = STATE_ACTIVE;
                    isCascadeStepOnePassed = false;
                    retSize = ( (MFCLASSIC_CMD_READ_RESPONSE_FRAME_SIZE + ISO14443A_CRCA_SIZE) * BITS_PER_BYTE );
                } else {
                    /* Unknown command. Enter HALT or IDLE state depending on origin */
                    State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
                    isCascadeStepOnePassed = false;
                }
            } else {
                /* Unknown command. Enter HALT or IDLE state depending on origin */
                State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
                isCascadeStepOnePassed = false;
            }
            break; /* End of state READY */

        case STATE_ACTIVE:
#ifdef SUPPORT_MF_CLASSIC_MAGIC_MODE
            if (Buffer[0] == MFCLASSIC_CMD_CHINESE_UNLOCK && AppMemoryUidMode()) {
                State = STATE_CHINESE_IDLE;
                Buffer[0] = MFCLASSIC_ACK_VALUE;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                break;
            }
#endif
            if ( (Buffer[0] == MFCLASSIC_CMD_AUTH_A) || (Buffer[0] == MFCLASSIC_CMD_AUTH_B) ) {
                if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_AUTH_FRAME_SIZE)) {
                    mfcHandleAuthenticationRequest(false, Buffer, &retSize);
                } else {
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO;
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                }
            } else {
                Buffer[0] = MFCLASSIC_NAK_TBKO_OPKO;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            }
            break; /* End of state ACTIVE */

        case STATE_AUTHING:
            if(isDetectionEnabled) {
#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
                // Save reader's auth phase 2 answer to our nonce from STATE_ACTIVE
                memcpy(DetectionDataSave+DETECTION_SAVE_P2_OFFSET, Buffer, DETECTION_READER_AUTH_P2_SIZE);
                // Align data storage in each KEYX dedicated memory space, and iterate counters
                uint8_t memSaveAddr;
                if (DetectionDataSave[DETECTION_KEYX_SAVE_IDX] == MFCLASSIC_CMD_AUTH_A) {
                    memSaveAddr = (DETECTION_MEM_DATA_START_ADDR + (DetectionAttemptsKeyA * DETECTION_BYTES_PER_SAVE));
                    DetectionAttemptsKeyA++;
                    DetectionAttemptsKeyA = DetectionAttemptsKeyA % DETECTION_MEM_MAX_KEYX_SAVES;
                } else {
                    memSaveAddr = (DETECTION_MEM_KEYX_SEPARATOR_OFFSET + (DetectionAttemptsKeyB * DETECTION_BYTES_PER_SAVE));
                    DetectionAttemptsKeyB++;
                    DetectionAttemptsKeyB = DetectionAttemptsKeyB % DETECTION_MEM_MAX_KEYX_SAVES;
                }
                // Write to app memory
                if(!isDetectionCanaryWritten) {
                    AppWorkingMemoryWrite(DetectionCanary, DETECTION_BLOCK0_CANARY_ADDR, DETECTION_BLOCK0_CANARY_SIZE);
                    isDetectionCanaryWritten = true;
                }
                AppWorkingMemoryWrite(DetectionDataSave, memSaveAddr, DETECTION_BYTES_PER_SAVE);
                State = STATE_ACTIVE;
#endif
            } else {
                /* Reader delivers an encrypted nonce. We use it
                * to setup the crypto1 LFSR in nonlinear feedback mode.
                * Furthermore it delivers an encrypted answer. Decrypt and check it */
                Crypto1Auth(&Buffer[0]);
                mfcDecryptBuffer(&Buffer[MFCLASSIC_MEM_NONCE_SIZE], MFCLASSIC_MEM_NONCE_SIZE);
                if ( !memcmp(&Buffer[MFCLASSIC_MEM_NONCE_SIZE], ReaderResponse, MFCLASSIC_MEM_NONCE_SIZE) ) {
                    /* Reader is authenticated. Encrypt the precalculated card response
                    * and generate the parity bits. */
                    mfcEncryptBuffer(Buffer, CardResponse, MFCLASSIC_MEM_NONCE_SIZE);
                    State = STATE_AUTHED_IDLE;
                    retSize = (MFCLASSIC_CMD_AUTH_BA_FRAME_SIZE * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
                } else {
		    /* Just reset on authentication error. */
                    State = STATE_IDLE;
		    /* In detection mode, communication can continue. */
                    if(isDetectionEnabled)
                        State = STATE_ACTIVE;

                }
            }
            break; /* End of state AUTHING */

        case STATE_AUTHED_IDLE:
            /* In this state, all communication is encrypted. Thus we first have to decrypt
            * the incoming data. */
            mfcDecryptBuffer(Buffer, MFCLASSIC_MEM_NONCE_SIZE);
            /* We could also get an encrypted HALT... */
            if ( !mfcHandleHaltCommand(Buffer, &retSize) ) {
                /* All possible operations at this state have all same frame size, so we do
                 * CRC check first */
                if (!ISO14443ACheckCRCA(Buffer, MFCLASSIC_CMD_COMMON_FRAME_SIZE)) {
                    State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO ^ Crypto1Nibble();
                    retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
                /* If CRC is valid we continue, with CRC passed */
                } else {
                    if (Buffer[0] == MFCLASSIC_CMD_READ) {
                        /* Read command. Read data from memory and append CRCA. */
                        /* Sector trailor? Use access conditions! */

			if ((Buffer[1] < 128 && (Buffer[1] & 3) == 3) || ((Buffer[1] & 15) == 15)) {
			    uint8_t Acc;
			    CurrentAddress = Buffer[1];
			    /* Decode the access conditions */
                            Acc = abTrailorAccessConditions[ GetAccessCondition(CurrentAddress) ][ KeyInUse ];

                            /* Prepare empty Block */
                            for (uint8_t i = 0; i < MFCLASSIC_MEM_BYTES_PER_BLOCK; i++)
                                Buffer[i] = 0;

                            /* Allways copy the GPB */
                            /* Key A can never be read! */
                            /* Access conditions were already read during authentication! */
                            Buffer[MFCLASSIC_MEM_KEY_SIZE + MFCLASSIC_MEM_ACC_GPB_SIZE - 1] = AccessConditions[MFCLASSIC_MEM_ACC_GPB_SIZE - 1];

			    /* Access conditions are already known */
                            if (Acc & MFCLASSIC_ACC_TRAILOR_READ_ACC) {
                                Buffer[MFCLASSIC_MEM_KEY_SIZE]     = AccessConditions[0];
                                Buffer[MFCLASSIC_MEM_KEY_SIZE + 1] = AccessConditions[1];
                                Buffer[MFCLASSIC_MEM_KEY_SIZE + 2] = AccessConditions[2];
                            }

                            /* Key B is readable in some rare cases */
                            if (Acc & MFCLASSIC_ACC_TRAILOR_READ_KEYB) {
                                AppCardMemoryRead(Buffer + MFCLASSIC_MEM_BYTES_PER_BLOCK - MFCLASSIC_MEM_KEY_SIZE,
                                            (uint16_t)(CurrentAddress | 3) * MFCLASSIC_MEM_BYTES_PER_BLOCK + MFCLASSIC_MEM_BYTES_PER_BLOCK - MFCLASSIC_MEM_KEY_SIZE,
                                            MFCLASSIC_MEM_KEY_SIZE);
                            }
                        } else {
			    AppCardMemoryRead(Buffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                        }
                        ISO14443AAppendCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                        /* Encrypt and calculate parity bits. */
                        mfcEncryptBuffer(Buffer, Buffer, (ISO14443A_CRCA_SIZE + MFCLASSIC_MEM_BYTES_PER_BLOCK));
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
                                AppCardMemoryWrite(BlockBuffer, (uint16_t) Buffer[1] * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
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
                        mfcHandleAuthenticationRequest(true, Buffer, &retSize);
                    } else {
                        /* Unknown command. Goes back to ACTIVE */
                        State = STATE_ACTIVE;
                    }
                } /* End of if/else CRC check condition */
            } /* End of possible encrypted HALT command */
            break; /* End of state AUTHED_IDLE */

        case STATE_WRITE:
            /* The reader has issued a write command earlier and is now
             * sending the data to be written. Decrypt the data first and
             * check for CRC. Then write the data when ReadOnly mode is not
             * activated. */
            /* We receive 16 bytes of data to be written and 2 bytes CRCA. Decrypt */
            mfcDecryptBuffer(Buffer, (MFCLASSIC_MEM_BYTES_PER_BLOCK + ISO14443A_CRCA_SIZE));
            /* We could also get an encrypted HALT... */
            if ( !mfcHandleHaltCommand(Buffer, &retSize) ) {
                if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_BYTES_PER_BLOCK)) {
                    /* Silently ignore in ReadOnly mode */
                    if (!ActiveConfiguration.ReadOnly) {
                        AppCardMemoryWrite(Buffer, CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
                    }
                    Buffer[0] = MFCLASSIC_ACK_VALUE ^ Crypto1Nibble();
                } else {
                    Buffer[0] = MFCLASSIC_NAK_TBOK_CRCKO ^ Crypto1Nibble();
                }
                State = STATE_AUTHED_IDLE;
                retSize = MFCLASSIC_ACK_NAK_FRAME_SIZE;
            }
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
            mfcDecryptBuffer(Buffer, (MFCLASSIC_MEM_VALUE_SIZE + ISO14443A_CRCA_SIZE));
            /* We could also get an encrypted HALT... */
            if ( !mfcHandleHaltCommand(Buffer, &retSize) ) {
                if (ISO14443ACheckCRCA(Buffer, MFCLASSIC_MEM_VALUE_SIZE)) {
                    AppCardMemoryRead(BlockBuffer, (uint16_t) CurrentAddress * MFCLASSIC_MEM_BYTES_PER_BLOCK, MFCLASSIC_MEM_BYTES_PER_BLOCK);
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
            }
            break; /* End of states DECREMENT, INCREMENT and RESTORE */

        default:
            /* Unknown state should never happen */
            retSize = ISO14443A_APP_NO_RESPONSE;
            State = isFromHaltChain ? STATE_HALT : STATE_IDLE;
        } /* End of states switch/case */
    } /* End of if/else WUPA/REQA condition */
#ifdef CONFIG_MF_CLASSIC_LOG_SUPPORT
    /* Log what goes from tag if logging enabled */
    if(isLogEnabled) {
        MifareClassicAppLogBufferLine(Buffer, (retSize & ISO14443A_APP_CUSTOM_PARITY) ? (retSize & ~ISO14443A_APP_CUSTOM_PARITY) : (retSize), MFCLASSIC_LOG_TAG);
    }
#endif
    return retSize;
}

void MifareClassicGetUid(ConfigurationUidType Uid) {
    if (is7BytesUID) {
        AppCardMemoryRead(&Uid[0], MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE-1);
        AppCardMemoryRead(&Uid[3], MFCLASSIC_MEM_UID_CL2_ADDRESS, MFCLASSIC_MEM_UID_CL2_SIZE);
    } else {
        AppCardMemoryRead(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
    }
}

void MifareClassicSetUid(ConfigurationUidType Uid) {
    if (is7BytesUID) {
        AppCardMemoryWrite(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, ActiveConfiguration.UidSize);
    } else {
        uint8_t BCC =  Uid[0] ^ Uid[1] ^ Uid[2] ^ Uid[3];
        AppCardMemoryWrite(Uid, MFCLASSIC_MEM_UID_CL1_ADDRESS, MFCLASSIC_MEM_UID_CL1_SIZE);
        AppCardMemoryWrite(&BCC, MFCLASSIC_MEM_UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
    }
}

void MifareClassicGetAtqa(uint16_t * Atqa) {
    *Atqa = CardATQAValue;
}

void MifareClassicSetAtqa(uint16_t Atqa) {
    CardATQAValue = Atqa;
}

void MifareClassicGetSak(uint8_t * Sak) {
    *Sak = CardSAKValue;
}

void MifareClassicSetSak(uint8_t Sak) {
    CardSAKValue = Sak;
}

#endif /* Compilation support */
