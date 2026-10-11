/*
 * LegicPrime.h
 *
 * LEGIC Prime card emulation, ChameleonMini-rebooted RevE port.
 * Ported from the RfidResearchGroup/proxmark3 legic_prng.c/legicrfsim.c
 * algorithms (real LFSR+mux keystream, not a fixed-RAND replay).
 */

#ifndef LEGICPRIME_H_
#define LEGICPRIME_H_

#include "../Common.h"
#include "../Configuration.h"

/* ---- Keystream generator (ported from common/legic_prng.c, unchanged) ---- */
typedef struct {
    uint8_t a; /* 7-bit LFSR, seeded with RAND */
    uint8_t b; /* 8-bit LFSR, seeded with (RAND<<1)|1 */
} Lfsr_t;

#define LEGIC_PRIME_UID_SIZE   4
/* The three card geometries proxmark3 knows (legicrfsim.c init_card):
 *   MIM22   cmdsize  6, addrsize 5,   22 bytes, type 0x0D, ack 0x19
 *   MIM256  cmdsize  9, addrsize 8,  256 bytes, type 0x1D, ack 0x39
 *   MIM1024 cmdsize 11, addrsize 10, 1024 bytes, type 0x3D, ack 0x39
 * One RAM working buffer sized for the largest is shared by all three. */
#define LEGIC_PRIME_MEM_22      22
#define LEGIC_PRIME_MEM_256     256
#define LEGIC_PRIME_MEM_1024    1024
#define LEGIC_PRIME_MEM_SIZE    LEGIC_PRIME_MEM_1024

#define LEGICPRIME_APP_NO_RESPONSE 0x0000

void LegicPrimeAppInit22(void);
void LegicPrimeAppInit256(void);
void LegicPrimeAppInit1024(void);
void LegicPrimeAppTask(void);
/* Called by ISO14443-2F.c during frame reception to advance the keystream over
 * the gap that preceded the frame, in bounded instalments. Kept out of
 * LegicPrimeAppProcess() so it does not run against the response deadline. */
void LegicPrimeAppGapForward(uint8_t Ticks);
void LegicPrimeLoadDefaultImage(void);
void LegicPrimeAppReset(void);
uint16_t LegicPrimeAppProcess(uint8_t *ByteBuffer, uint16_t BitCount);
void LegicPrimeGetUid(ConfigurationUidType Uid);
void LegicPrimeSetUid(ConfigurationUidType Uid);


#endif /* LEGICPRIME_H_ */
