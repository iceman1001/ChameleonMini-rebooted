/*
 * LegicPrime.c
 *
 * LEGIC Prime card emulation, ChameleonMini-rebooted RevE port.
 *
 * Implements the setup and main phases of the protocol described in
 * SAR-PR-2011-03 (Ploetz & Nohl, "Peeling Away Layers of an RFID Security
 * System"): a 7-bit RAND from the reader, a 6-bit card type, a 6-bit ack,
 * then 9- or 11-bit read commands answered with 12-bit responses. Every frame
 * is obfuscated with the LFSR+mux keystream seeded from that RAND, and read
 * responses carry a transport CRC-4.
 *
 * The keystream, the tick schedule and the CRC-4 are ported from proxmark3
 * armsrc/legicrfsim.c and armsrc/legicrf.c, which are the reference for both
 * ends of the exchange. ISO14443-2F.c owns the physical layer.
 *
 * MIM22, MIM256 and MIM1024 are all supported, selected by the active
 * configuration in the same way proxmark3 selects with hf legic sim --22 /
 * --256 / --1024:
 *
 *   CONFIG=LEGIC_PRIME_256      (or _22 / _1024)
 *   UID=11223344                MCC (byte 4) is recomputed automatically
 *   LEGICLOAD                   write a blank card into this slot
 *
 * Card contents live in the slot's card memory, so the standard UPLOAD and
 * DOWNLOAD commands load and save dumps, the equivalent of the proxmark's
 * hf legic sim -f <file>. A slot is never auto-seeded.
 */

#include "LegicPrime.h"
#include "../Codec/ISO14443-2F.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "../Memory/Memory.h"

/* The three geometries proxmark3 simulates (`hf legic sim --22/--256/--1024`),
 * taken from legicrfsim.c init_card() and setup_phase(). Selected at runtime by
 * the active configuration, exactly as the proxmark selects by flag. */
typedef struct {
    uint8_t  TypeFrame;   /* 6-bit type response we send */
    uint8_t  AckExpected; /* 6-bit ack the reader sends back */
    uint8_t  CmdSize;     /* command frame width: 1 read/write bit + address */
    uint16_t CardSize;    /* bytes */
} LegicProfileType;

static const LegicProfileType ProfileMIM22   = { 0x0D, 0x19,  6, LEGIC_PRIME_MEM_22   };
static const LegicProfileType ProfileMIM256  = { 0x1D, 0x39,  9, LEGIC_PRIME_MEM_256  };
static const LegicProfileType ProfileMIM1024 = { 0x3D, 0x39, 11, LEGIC_PRIME_MEM_1024 };

static LegicProfileType Profile;

/* Keystream tick budget between frames.
 *
 * The generator does not free-run in real time: both sides advance it by a
 * fixed, frame-dependent number of ticks, one per bit sent or received plus a
 * fixed gap per turnaround. The schedule is stated literally by the reader in
 * armsrc/legicrf.c:
 *
 *   setup: prng_init(iv); forward(2); rx_frame(6);  forward(3); tx_frame(ack,6)
 *   read:  forward(2); tx_frame(cmd,9);  forward(2); rx_frame(12)
 *   write: forward(2); tx_frame(cmd,21); forward(3); rx_ack()
 *
 * From the card's side: forward 2 before answering, 3 before de-obfuscating
 * the ack that follows our answer, and 2 before de-obfuscating a command.
 * legicrfsim.c reaches the same positions with a real-time wait loop; these
 * fixed counts are the exact equivalent.
 *
 * These are prng ticks, not hardware time; ISO14443-2F.c owns the physical
 * delays. */
#define LEGIC_TX_GAP_TICKS        2   /* RX end -> our answer */
#define LEGIC_RX_AFTER_TX_TICKS   3   /* our answer -> reader's ack */
#define LEGIC_RX_GAP_TICKS        2   /* reader frame -> next reader frame */
/* Write-ACK gap: legicrfsim.c tx_ack() forwards TAG_ACK_WAIT/TAG_BIT_PERIOD-1
 * = 758/21-1 = 35. The reader's rx_ack() polls once per bit period forwarding
 * one tick each until it sees the ack, so this count and ISO14443-2F.c's
 * physical 3.57ms delay have to agree or the two ends desync after a write. */
#define LEGIC_WRITE_ACK_GAP_TICKS 35
/* Extra tick the reader takes at the end of a successful read (legicrf.c
 * read_byte). Not part of any frame -- purely a post-read adjustment. */
#define LEGIC_READ_TRAILER_TICKS  1

/* ---- Keystream generator (ported from common/legic_prng.c, unchanged) ---- */
static struct {
    uint8_t a; /* 7-bit LFSR, seeded with RAND */
    uint8_t b; /* 8-bit LFSR, seeded with (RAND<<1)|1 */
} Lfsr;

static void legic_prng_init(uint8_t iv) {
    Lfsr.a = iv;
    Lfsr.b = iv ? ((iv << 1) | 1) : 0; /* iv==0: force an all-zero stream */
}

static void legic_prng_forward(uint8_t count) {
    while (count--) {
        Lfsr.a = (Lfsr.a >> 1 | (Lfsr.a ^ Lfsr.a >> 6) << 6) & 0x7F;
        Lfsr.b = Lfsr.b >> 1 | (Lfsr.b ^ Lfsr.b >> 2 ^ Lfsr.b >> 3 ^ Lfsr.b >> 7) << 7;
    }
}

static uint8_t legic_prng_get_bit(void) {
    uint8_t idx = 7 - ((Lfsr.a & 4) | (Lfsr.a >> 2 & 2) | (Lfsr.a >> 4 & 1));
    return Lfsr.b >> idx & 1;
}

/* XOR the low `n` bits of `value` against the next `n` keystream bits,
 * consuming (advancing) exactly one tick per bit -- symmetric, so this is
 * used for both de-obfuscating received frames and obfuscating replies. */
static uint32_t legic_xor_bits(uint32_t value, uint8_t n) {
    uint32_t out = 0;
    for (uint8_t i = 0; i < n; i++) {
        out |= (uint32_t)(((value >> i) & 1) ^ legic_prng_get_bit()) << i;
        legic_prng_forward(1);
    }
    return out;
}

/* ---- Transport CRC-4: width 4, poly 0xC, init 0x5, not reflected. Ported
 * from armsrc/legicrfsim.c calc_crc4(), not from common/crc.c CRC4Legic(),
 * which is a different implementation and does not match real hardware. ---- */
static uint8_t calc_crc4(uint16_t cmd, uint8_t cmd_sz, uint8_t value) {
    uint8_t state = 0x5;
    uint32_t data = ((uint32_t)value << cmd_sz) | cmd;
    for (uint8_t i = 0; i < 8 + cmd_sz; i++) {
        uint8_t old = state;
        state >>= 1;
        if ((old ^ data) & 1) {
            state ^= 0xC;
        }
        data >>= 1;
    }
    return state & 0xF;
}

/* ---- Bit-packed buffer helpers (LSBit-first, matches CodecBuffer layout) */
static uint32_t GetBits(const uint8_t *buf, uint8_t n) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < n; i++) {
        if (buf[i / 8] & (1 << (i % 8))) {
            v |= (uint32_t)1 << i;
        }
    }
    return v;
}

static void SetBits(uint8_t *buf, uint32_t v, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        if ((v >> i) & 1) {
            buf[i / 8] |= (1 << (i % 8));
        } else {
            buf[i / 8] &= ~(1 << (i % 8));
        }
    }
}

/* ---- Card memory + session state ---- */
static uint8_t LegicMem[LEGIC_PRIME_MEM_SIZE];

typedef enum {
    LEGIC_STATE_WAIT_IV,
    LEGIC_STATE_WAIT_ACK,
    LEGIC_STATE_CONNECTED
} LegicStateType;

static LegicStateType State;
typedef struct { uint8_t a, b; } LfsrState;
static void ks_init(LfsrState *st, uint8_t iv);
static void ks_fwd(LfsrState *st, uint8_t n);
static uint8_t ks_bit(LfsrState *st);
static uint32_t ks_xor(LfsrState *st, uint32_t v, uint8_t n);
static uint8_t  Crc8Legic(const uint8_t *data, uint8_t len);

static volatile uint16_t ReadCounter;   /* next read index the reader will ask for */
static volatile bool     AckSeen;        /* MIM22's 6-bit read == ACK width; the ACK is the first 6-bit frame, reads follow */
/* The reader reads sequentially, so rather than precomputing a response per
 * card byte, keep the session keystream state and compute each 12-bit response
 * just-in-time into a small ring buffer that the main loop keeps ahead of
 * ReadCounter. That is O(ring) RAM instead of O(cardsize), which is what lets
 * MIM1024 fit in 4KB of SRAM. The interrupt path only serves the ring. */
#define LEGIC_RING_SIZE 32           /* power of 2; window ahead of the reader */
static volatile uint16_t Ring[LEGIC_RING_SIZE];
static volatile uint16_t RingFilled;  /* absolute count of responses computed */
static volatile uint8_t  SessIv;
static volatile bool     SessReset;   /* ISR asks the main loop to (re)start fill */
static LfsrState         SessKs;      /* main-loop-owned keystream */
static uint16_t          FillIdx;
static volatile int16_t PendingWriteAddr = -1;

/* Card contents live in the Chameleon's own card memory, so the standard
 * UPLOAD / DOWNLOAD terminal commands load and save dumps -- the equivalent of
 * proxmark3's `hf legic sim -f <fn>`. It is mirrored into RAM here because the
 * protocol runs in interrupt context and must not touch SPI flash there.
 *
 * Nothing is auto-seeded; use LEGICLOAD to write a blank card into the slot. */
static void LegicPrimeAppInitCommon(const LegicProfileType *p) {
    State = LEGIC_STATE_WAIT_IV;
    Profile = *p;

    memset(LegicMem, 0x00, sizeof(LegicMem));
    AppCardMemoryRead(LegicMem, 0, Profile.CardSize);
}

/* Write a blank card into this slot's card memory, overwriting whatever it
 * held. Byte 4 is the MCC, derived from the UID; bytes 5 and 6 are the DCF,
 * 0xFFFF marking an ordinary card rather than a master token. The rest is
 * zeroed. Load a real dump with UPLOAD, or set the UID with the UID command. */
void LegicPrimeLoadDefaultImage(void) {
    uint16_t Size = Profile.CardSize ? Profile.CardSize : LEGIC_PRIME_MEM_1024;
    static const uint8_t BlankUid[LEGIC_PRIME_UID_SIZE] = { 0x01, 0x02, 0x03, 0x04 };

    memset(LegicMem, 0x00, Size);
    memcpy(LegicMem, BlankUid, LEGIC_PRIME_UID_SIZE);
    LegicMem[4] = Crc8Legic(LegicMem, LEGIC_PRIME_UID_SIZE);
    LegicMem[5] = 0xFF;
    LegicMem[6] = 0xFF;
    AppCardMemoryWrite(LegicMem, 0, Size);
}

void LegicPrimeAppInit22(void)   { LegicPrimeAppInitCommon(&ProfileMIM22); }
void LegicPrimeAppInit256(void)  { LegicPrimeAppInitCommon(&ProfileMIM256); }
void LegicPrimeAppInit1024(void) { LegicPrimeAppInitCommon(&ProfileMIM1024); }

/* Written bytes are flushed here rather than from LegicPrimeAppProcess, which
 * runs inside the RX-end interrupt and must not block on SPI flash. */
void LegicPrimeAppTask(void) {
    if (SessReset) {
        SessReset = false;
        ks_init(&SessKs, SessIv);
        ks_fwd(&SessKs, LEGIC_TX_GAP_TICKS);
        ks_xor(&SessKs, Profile.TypeFrame, 6);
        ks_fwd(&SessKs, LEGIC_RX_AFTER_TX_TICKS);
        ks_xor(&SessKs, 0, 6);
        FillIdx = 0;
        RingFilled = 0;
    }
    while (FillIdx < Profile.CardSize && FillIdx < (uint16_t)(ReadCounter + LEGIC_RING_SIZE)) {
        ks_fwd(&SessKs, LEGIC_RX_GAP_TICKS);
        ks_xor(&SessKs, 0, Profile.CmdSize);
        uint8_t  data = LegicMem[FillIdx];
        uint8_t  crc  = calc_crc4((uint16_t)((FillIdx << 1) | 1), Profile.CmdSize, data);
        uint32_t plain = ((uint32_t)crc << 8) | data;
        ks_fwd(&SessKs, LEGIC_TX_GAP_TICKS);
        Ring[FillIdx & (LEGIC_RING_SIZE - 1)] = (uint16_t)ks_xor(&SessKs, plain, 12);
        ks_fwd(&SessKs, LEGIC_READ_TRAILER_TICKS);
        FillIdx++;
        RingFilled = FillIdx;
    }
    if (PendingWriteAddr >= 0) {
        uint16_t Addr = (uint16_t)PendingWriteAddr;
        PendingWriteAddr = -1;
        AppCardMemoryWrite(&LegicMem[Addr], Addr, 1);
    }
}

void LegicPrimeAppReset(void) {
    State = LEGIC_STATE_WAIT_IV;
}

/* Local-state keystream (same LFSR+mux as above) for the main-loop precompute,
 * so it never races the ISR's global Lfsr. */
static void ks_init(LfsrState *st, uint8_t iv) { st->a = iv; st->b = iv ? ((iv << 1) | 1) : 0; }
static void ks_fwd(LfsrState *st, uint8_t n) {
    while (n--) {
        st->a = (st->a >> 1 | (st->a ^ st->a >> 6) << 6) & 0x7F;
        st->b = st->b >> 1 | (st->b ^ st->b >> 2 ^ st->b >> 3 ^ st->b >> 7) << 7;
    }
}
static uint8_t ks_bit(LfsrState *st) {
    uint8_t idx = 7 - ((st->a & 4) | (st->a >> 2 & 2) | (st->a >> 4 & 1));
    return st->b >> idx & 1;
}
static uint32_t ks_xor(LfsrState *st, uint32_t v, uint8_t n) {
    uint32_t o = 0;
    for (uint8_t i = 0; i < n; i++) { o |= (uint32_t)(((v >> i) & 1) ^ ks_bit(st)) << i; ks_fwd(st, 1); }
    return o;
}

uint16_t LegicPrimeAppProcess(uint8_t *Buffer, uint16_t BitCount) {

    /* 7-bit RAND: start of a session. Send the type from the ISR (fast) and
     * ask the main loop to precompute all read responses for this IV. */
    if (BitCount == 7) {
        uint8_t iv = (uint8_t)GetBits(Buffer, 7);
        legic_prng_init(iv);
        State = LEGIC_STATE_CONNECTED;
        ReadCounter = 0;
        RingFilled = 0;
        AckSeen = false;
        SessIv = iv;
        SessReset = true;       /* main loop starts streaming this session */
        legic_prng_forward(LEGIC_TX_GAP_TICKS);
        uint32_t obf = legic_xor_bits(Profile.TypeFrame, 6);
        SetBits(Buffer, obf, 6);
        return 6;
    }

    if (BitCount == 6 && !AckSeen) { /* the ACK (once, after the type) */
        AckSeen = true;
        return LEGICPRIME_APP_NO_RESPONSE;
    }

    if (State != LEGIC_STATE_CONNECTED) {
        return LEGICPRIME_APP_NO_RESPONSE;
    }

    /* Read command: serve the precomputed response by sequential index. If the
     * buffer is not ready yet (main loop still computing), stay silent; the
     * reader simply retries the session. */
    if (BitCount == Profile.CmdSize) {
        if (ReadCounter < RingFilled) {
            SetBits(Buffer, Ring[ReadCounter & (LEGIC_RING_SIZE - 1)], 12);
            ReadCounter++;
            return 12;
        }
        return LEGICPRIME_APP_NO_RESPONSE;   /* fill fell behind (should not) */
    }

    return LEGICPRIME_APP_NO_RESPONSE;
}

void LegicPrimeGetUid(ConfigurationUidType Uid) {
    memcpy(Uid, LegicMem, LEGIC_PRIME_UID_SIZE);
}

/* CRC-8/LEGIC (poly 0x63, init 0x55, reflected in/out) over the 4 UID bytes,
 * matching proxmark3 common/crc.c CRC8Legic. This is the MCC (byte 4). */
static uint8_t Crc8Legic(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x55;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 1) ? (uint8_t)((crc >> 1) ^ 0xC6) : (uint8_t)(crc >> 1);
        }
    }
    uint8_t r = 0;
    for (uint8_t i = 0; i < 8; i++) r = (uint8_t)((r << 1) | ((crc >> i) & 1));
    return r;
}

void LegicPrimeSetUid(ConfigurationUidType Uid) {
    memcpy(LegicMem, Uid, LEGIC_PRIME_UID_SIZE);
    LegicMem[4] = Crc8Legic(LegicMem, LEGIC_PRIME_UID_SIZE);  /* MCC secures the UID */
    AppCardMemoryWrite(LegicMem, 0, 5);
}
