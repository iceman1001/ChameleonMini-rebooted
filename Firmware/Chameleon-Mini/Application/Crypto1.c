#include "Crypto1.h"

/* avoid compiler complaining at the shift macros */
#pragma GCC diagnostic ignored "-Wuninitialized"

#define PRNG_MASK        0x002D0000UL
/* x^16 + x^14 + x^13 + x^11 + 1 */

#define PRNG_SIZE        4 /* Bytes */
#define NONCE_SIZE       4 /* Bytes */

#define LFSR_MASK_EVEN    0x2010E1UL
#define LFSR_MASK_ODD    0x3A7394UL
/* x^48 + x^43 + x^39 + x^38 + x^36 + x^34 + x^33 + x^31 + x^29 +
 * x^24 + x^23 + x^21 + x^19 + x^13 + x^9 + x^7 + x^6 + x^5 + 1 */

#define LFSR_SIZE        6 /* Bytes */

const uint8_t TableA[32] PROGMEM = { // for first, third and fourth
	0,0,1,1,0,1,0,0,
	0,1,0,0,1,1,1,1,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};

const uint8_t TableB[32] PROGMEM = { // for second and fifth
	0,0,0,1,1,1,0,0,
	1,0,0,1,1,0,1,1,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};

uint32_t StateOdd;
uint32_t StateEven;

static uint8_t Crypto1ByteAuth(uint8_t In, uint8_t AuthSet)
{
	uint8_t KeyStream = 0;
	uint8_t i;

	/* Generate 8 keystream-bits */
	for (i=0; i<8; i++) {

		/* Calculate next bit and add to KeyStream */
		uint8_t Out = Crypto1Bit(In&1,AuthSet);

		In>>=1;
		KeyStream>>=1;
		if(Out) {
			KeyStream |= (1<<7);
		}
		
	}

	return KeyStream;
}

void Crypto1Setup(uint8_t Key[6], uint8_t Uid[4], uint8_t CardNonce[4])
{

	StateOdd = 0;
	StateEven = 0;

	int i = 0, j = 0;
	for(i = 0 ; i < 6 ; i++) {

		for(j = 7 ; j > 0 ; j -= 2) {
			StateOdd = StateOdd << 1 | (Key[i]>>((j-1) ^ 7)&1);
			StateEven = StateEven << 1 | (Key[i]>>((j) ^ 7)&1);
		}
	}

	for(i=0; i<4; i++) {
		CardNonce[i] ^= Crypto1ByteAuth(Uid[i] ^ CardNonce[i], 0);
	}

}

void Crypto1Auth(uint8_t EncryptedReaderNonce[4])
{
	uint8_t i;

	/* Calculate Authentication on Nonce */
	for(i = 0 ; i < 4 ; i++) {
		Crypto1ByteAuth(EncryptedReaderNonce[i],1);
	}
}

uint8_t Crypto1Byte(void)
{
	uint8_t KeyStream = 0;
	uint8_t i;

	/* Generate 8 keystream-bits */
	for (i=0; i<8; i++) {

		/* Calculate next bit and add to KeyStream */
		uint8_t Out = Crypto1Bit(0,0);

		KeyStream>>=1;
		if(Out) {
			KeyStream |= (1<<7);
		}
		
	}

	return KeyStream;
}

uint8_t Crypto1Nibble(void)
{
	uint8_t KeyStream = 0;
	uint8_t i;

	/* Generate 4 keystream-bits */
	for (i=0; i<4; i++) {

		/* Calculate next bit and add to KeyStream */
		uint8_t Out = Crypto1Bit(0,0);

		KeyStream>>=1;
		if(Out) {
			KeyStream |= (1<<7);
		}
		
	}

	return KeyStream;
}

void Crypto1PRNG(uint8_t State[4], uint16_t ClockCount)
{
	while(ClockCount--) {
		/* Actually, the PRNG is a 32 bit register with the upper 16 bit
		* used as a LFSR. Furthermore only mask-byte 2 contains feedback at all.
		* We rely on the compiler to optimize this for us here.
		* XOR all tapped bits to a single feedback bit. */
		uint8_t Feedback = 0;

		Feedback ^= State[0] & (uint8_t) (PRNG_MASK >> 0);
		Feedback ^= State[1] & (uint8_t) (PRNG_MASK >> 8);
		Feedback ^= State[2] & (uint8_t) (PRNG_MASK >> 16);
		Feedback ^= State[3] & (uint8_t) (PRNG_MASK >> 24);

		Feedback ^= Feedback >> 4;
		Feedback ^= Feedback >> 2;
		Feedback ^= Feedback >> 1;

		/* For ease of processing convert the state into a 32 bit integer first */
		uint32_t Temp = 0;

		Temp |= (uint32_t) State[0] << 0;
		Temp |= (uint32_t) State[1] << 8;
		Temp |= (uint32_t) State[2] << 16;
		Temp |= (uint32_t) State[3] << 24;

		/* Cycle LFSR and feed back. */
		Temp >>= 1;

		if (Feedback & 0x01) {
			Temp |= (uint32_t) 1 << (8 * PRNG_SIZE - 1);
		}

		/* Store back state */
		State[0] = (uint8_t) (Temp >> 0);
		State[1] = (uint8_t) (Temp >> 8);
		State[2] = (uint8_t) (Temp >> 16);
		State[3] = (uint8_t) (Temp >> 24);
	}


}
