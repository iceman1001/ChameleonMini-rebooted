#include "Detection.h"
#include <string.h>


#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "../LED.h"
#include "../Random.h"

 #define MFCLASSIC_1K_ATQA_VALUE     0x0004
 #define MFCLASSIC_1K_SAK_CL1_VALUE  0x08
 
 #define MEM_UID_CL1_ADDRESS         0x00
 #define MEM_UID_CL1_SIZE            4

 #define CMD_HALT                    0x50
 #define CMD_HALT_FRAME_SIZE         2        /* Bytes without CRCA */
 #define CMD_AUTH_A                  0x60
 #define CMD_AUTH_B                  0x61
 #define CMD_AUTH_FRAME_SIZE         2         /* Bytes without CRCA */
 #define CMD_AUTH_RB_FRAME_SIZE      4        /* Bytes */
 #define CMD_AUTH_BA_FRAME_SIZE      4        /* Bytes */
 #define CMD_READ                    0x30
 #define CMD_WRITE                   0xA0
 #define CMD_DECREMENT               0xC0
 #define CMD_INCREMENT               0xC1
 #define CMD_RESTORE                 0xC2
 #define CMD_TRANSFER                0xB0

 #define NAK_INVALID_ARG             0x00
 #define NAK_CRC_ERROR               0x01
 #define NAK_NOT_AUTHED              0x04

 #define ACK_NAK_FRAME_SIZE          4         /* Bits */

 #define MEM_SECTOR_ADDR_MASK        0x3C
 #define MEM_BYTES_PER_BLOCK         16        /* Bytes */
 static enum {
	 STATE_HALT,
	 STATE_IDLE,
	 STATE_READY,
	 STATE_ACTIVE,
	 STATE_AUTHING,
	 STATE_AUTHED_IDLE,
	 STATE_WRITE,
	 STATE_INCREMENT,
	 STATE_DECREMENT,
	 STATE_RESTORE
 } State;

 static uint16_t CardATQAValue;
 static uint8_t CardSAKValue;
 uint8_t data_svae[16];

 uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount)
 {
	static uint8_t turn_falga=0;
	static uint8_t turn_falgb=0;
	static uint8_t keyb_falg=0;
	//侦测卡保存
	if(State==STATE_AUTHING)
	{
		State=STATE_IDLE;
		//储存信息
		memcpy(data_svae+8,Buffer,8);

		if(!keyb_falg)
		{
		MemoryWriteBlock(data_svae, (turn_falga+1) * MEM_BYTES_PER_BLOCK+4096, MEM_BYTES_PER_BLOCK);
		turn_falga= (++turn_falga)%6;
		}	
		else
		{
		MemoryWriteBlock(data_svae, (turn_falgb) * MEM_BYTES_PER_BLOCK + 112+4096, MEM_BYTES_PER_BLOCK);
		turn_falgb= (++turn_falgb)%6;
		}
				
		LEDPulse(LED_RED);
	}
	else
	{
		//发来0x26
		if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue))
		return BitCount;
		//发来0x93 0x20 或 0x93 0x70
		else if (Buffer[0] == ISO14443A_CMD_SELECT_CL1)
		{
			uint8_t UidCL1[4];
			MemoryReadBlock(UidCL1, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
			if (ISO14443ASelect(Buffer, &BitCount, UidCL1, CardSAKValue))
			return BitCount;
		}
		else if ( (Buffer[0] == CMD_AUTH_A) || (Buffer[0] == CMD_AUTH_B))
		{
			
			uint8_t CardNonce[4];
			/* Generate a random nonce and read UID and key from memory */
			RandomGetBuffer(CardNonce, sizeof(CardNonce));
			memcpy(data_svae,Buffer,4);
			memcpy(data_svae+4,CardNonce,4);

			if(Buffer[0] == CMD_AUTH_B) keyb_falg=1; 
			else keyb_falg=0;

			memcpy(Buffer,CardNonce,4);
			State = STATE_AUTHING;

			return CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
		}

	}

    return ISO14443A_APP_NO_RESPONSE;
 }

 void MifareDetectionInit(void)
 {
     //这里可以改下，因为是函数指针。
	 State = STATE_IDLE;
	 CardATQAValue = MFCLASSIC_1K_ATQA_VALUE;
	 CardSAKValue = MFCLASSIC_1K_SAK_CL1_VALUE;
 }

 void MifareDetectionReset(void)
 {
	 State = STATE_IDLE;
 }