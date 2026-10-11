/*
 * LegicPrime.c
 *
 * LEGIC Prime card emulation, ChameleonMini-rebooted RevE port.
 *
 * Implements the setup and main phases of the protocol described in
 * SAR-PR-2011-03 (Ploetz & Nohl, "Peeling Away Layers of an RFID Security
 * System"): a 7-bit RAND from the reader, a 6-bit card type, a 6-bit ack,
 * then read and write commands addressed by byte index. Every frame is
 * obfuscated with the LFSR+mux keystream seeded from that RAND, and read
 * responses carry a transport CRC-4.
 *
 * The keystream and the CRC-4 are ported from proxmark3 armsrc/legicrfsim.c
 * and armsrc/legicrf.c, which are the reference for both ends of the
 * exchange. ISO14443-2F.c owns the physical layer and measures the
 * reader-controlled inter-frame gaps the keystream schedule depends on.
 *
 * MIM22, MIM256 and MIM1024 are all supported, selected by the active
 * configuration in the same way proxmark3 selects with hf legic sim --22 /
 * --256 / --1024:
 *
 *   CONFIG=LEGIC_PRIME_256      (or _22 / _1024)
 *   UID=11223344                MCC (byte 4) is recomputed automatically
 *   LEGICLOAD                   write a blank card into this slot
 *
 * Reading with a proxmark when field-powered: use hf legic reader -@ (continuous);
 * a single-shot read can come back silent before the card has powered up.
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
#include "LegicPrimeHelper.h"

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

/* Bit 0 of a command word: set = read, clear = write. Address is the rest. */
#define LEGIC_CMD_READ            0x01

/* Keystream tick budget.
 *
 * Two kinds of interval have to be accounted for:
 *
 *  - Frame bits. Exactly one tick each, regardless of whether the bit is
 *    physically 60us (a 0) or 100us (a 1) on the reader's side.
 *    legic_xor_bits() consumes them as it goes.
 *
 *  - Turnarounds. The generator runs on with elapsed time, so this is
 *    whole bit periods measured by the codec (LegicRxGapPeriods), plus one
 *    extra tick when the frame before the turnaround was our own.
 *
 * The bias is not guesswork: it is what makes a proxmark3 and a real LEGIC
 * lock agree, across six turnarounds taken from two hf sniff captures. See the
 * table in ISO14443-2F.c. It also explains why fixed counts appeared to work
 * -- a proxmark3 paces every turnaround at 220us, which always truncates to 2
 * periods, so 2 and 2+1 look like constants. A lock that takes 355us and 523us
 * needs 3 and 5+1 instead, and fixed counts put the keystream one tick out on
 * the very first read command.
 *
 * Our own two slots stay constant because we control their length. */
#define LEGIC_TX_GAP_TICKS        2   /* our 330us response slot   (70/21 - 1) */
#define LEGIC_WRITE_ACK_GAP_TICKS 35  /* our 3.57ms ack slot      (758/21 - 1) */
#define LEGIC_GAP_AFTER_TX_BIAS   1   /* our frame came last */

Lfsr_t Lfsr;

static void legic_prng_init(uint8_t iv) {
    Lfsr.a = iv;
    Lfsr.b = iv ? ((iv << 1) | 1) : 0; /* iv==0: force an all-zero stream */
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

/* ---- Bit-packed buffer helpers (LSBit-first, matches CodecBuffer layout).
 * A walking byte pointer and a rotating mask, for the reason given above. */
static uint32_t GetBits(const uint8_t *buf, uint8_t n) {
    uint32_t v = 0;
    uint32_t bit = 1;
    uint8_t mask = 1;

    while (n--) {
        if (*buf & mask) {
            v |= bit;
        }
        bit <<= 1;
        if ((mask <<= 1) == 0) {
            mask = 1;
            buf++;
        }
    }
    return v;
}

static void SetBits(uint8_t *buf, uint32_t v, uint8_t n) {
    uint8_t mask = 1;

    while (n--) {
        if (v & 1) {
            *buf |= mask;
        } else {
            *buf &= (uint8_t)~mask;
        }
        v >>= 1;
        if ((mask <<= 1) == 0) {
            mask = 1;
            buf++;
        }
    }
}

/* ---- Card memory + session state ---- */
static uint8_t LegicMem[LEGIC_PRIME_MEM_SIZE];

typedef enum {
    LEGIC_STATE_WAIT_IV,    /* no session: waiting for a 7-bit RAND */
    LEGIC_STATE_WAIT_ACK,   /* type frame sent, reader's ack due next */
    LEGIC_STATE_CONNECTED   /* serving read and write commands */
} LegicStateType;

static volatile LegicStateType State;
static volatile int16_t PendingWriteAddr = -1;
static uint8_t  Crc8Legic(const uint8_t *data, uint8_t len);

/* Last de-obfuscated setup ack, for diagnosing a reader whose ack differs from
 * the profile's expectation. Not acted on; see the WAIT_ACK branch. */
static volatile uint8_t LastAck;

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
    if (PendingWriteAddr >= 0) {
        uint16_t Addr = (uint16_t)PendingWriteAddr;
        PendingWriteAddr = -1;
        AppCardMemoryWrite(&LegicMem[Addr], Addr, 1);
    }
}

void LegicPrimeAppReset(void) {
    State = LEGIC_STATE_WAIT_IV;
}


/* Called from the RX-end interrupt, with roughly 190us before the response
 * slot opens (see LEGIC_GAP_MAX_TICKS in ISO14443-2F.c). Everything here is
 * computed just in time for this one frame: the reader addresses bytes in
 * whatever order it likes, and the gap it just took is only knowable after the
 * fact, so neither the data nor the keystream position can be precomputed. */
uint16_t LegicPrimeAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    /* 7-bit RAND: start of a session. Nothing preceded it that the keystream
     * cares about, so the measured gap is deliberately ignored here. */
    if (BitCount == 7) {
        legic_prng_init((uint8_t)GetBits(Buffer, 7));
        State = LEGIC_STATE_WAIT_ACK;
        legic_prng_forward(LEGIC_TX_GAP_TICKS);
        SetBits(Buffer, legic_xor_bits(Profile.TypeFrame, 6), 6);
        return 6;
    }

    if (State == LEGIC_STATE_WAIT_IV) {
        return LEGICPRIME_APP_NO_RESPONSE;
    }

    /* A reader that went quiet for a long stretch cannot be assumed to have
     * kept count; carrying on would emit plausible-looking garbage. */
    if (LegicRxGapPeriods == LEGIC_GAP_DESYNC) {
        State = LEGIC_STATE_WAIT_IV;
        return LEGICPRIME_APP_NO_RESPONSE;
    }

    /* Pay for the turnaround that preceded this frame. */
    legic_prng_forward(LegicRxGapPeriods
                       + (LegicGapAfterOurFrame ? LEGIC_GAP_AFTER_TX_BIAS : 0));

    if (State == LEGIC_STATE_WAIT_ACK) {
        if (BitCount != 6) {
            State = LEGIC_STATE_WAIT_IV;
            return LEGICPRIME_APP_NO_RESPONSE;
        }
        /* De-obfuscated for the keystream's sake, and recorded, but a mismatch
         * is not treated as fatal: the ack values in the profiles come from
         * proxmark3's table and an unknown reader may legitimately use another
         * one. Rejecting on it would trade a wrong byte for no session at all. */
        LastAck = (uint8_t)legic_xor_bits(GetBits(Buffer, 6), 6);
        State = LEGIC_STATE_CONNECTED;
        return LEGICPRIME_APP_NO_RESPONSE;
    }

    /* Read: one command bit plus the address. */
    if (BitCount == Profile.CmdSize) {
        uint16_t Cmd = (uint16_t)legic_xor_bits(GetBits(Buffer, Profile.CmdSize),
                                                Profile.CmdSize);
        uint16_t Addr = Cmd >> 1;
        uint8_t  Data;
        uint32_t Plain;

        if (!(Cmd & LEGIC_CMD_READ) || Addr >= Profile.CardSize) {
            return LEGICPRIME_APP_NO_RESPONSE;
        }
        Data  = LegicMem[Addr];
        /* The CRC-4 covers the command word as sent, so it is only right if
         * the address really is the one the reader asked for. */
        Plain = ((uint32_t)calc_crc4(Cmd, Profile.CmdSize, Data) << 8) | Data;
        legic_prng_forward(LEGIC_TX_GAP_TICKS);
        SetBits(Buffer, legic_xor_bits(Plain, 12), 12);
        return 12;
    }

    /* Write: the same command word, then 8 data bits and a CRC-4 over both.
     * 18 bits on MIM22, 21 on MIM256, 23 on MIM1024 -- all within the codec's
     * RX_MAX_BITS. Unverified against a real reader; see the notes in the
     * commit. Answered with a single ack bit in the 3.57ms slot, which the
     * codec stretches on seeing a one-bit answer. */
    if (BitCount == (uint16_t)Profile.CmdSize + 12) {
        uint32_t Frame = legic_xor_bits(GetBits(Buffer, (uint8_t)BitCount),
                                        (uint8_t)BitCount);
        uint16_t Cmd  = (uint16_t)(Frame & (((uint32_t)1 << Profile.CmdSize) - 1));
        uint8_t  Data = (uint8_t)(Frame >> Profile.CmdSize);
        uint8_t  Crc  = (uint8_t)((Frame >> (Profile.CmdSize + 8)) & 0x0F);
        uint16_t Addr = Cmd >> 1;

        if ((Cmd & LEGIC_CMD_READ) || Addr >= Profile.CardSize ||
                calc_crc4(Cmd, Profile.CmdSize, Data) != Crc) {
            return LEGICPRIME_APP_NO_RESPONSE;
        }
        LegicMem[Addr] = Data;
        PendingWriteAddr = (int16_t)Addr;   /* flushed to flash by the task */

        legic_prng_forward(LEGIC_WRITE_ACK_GAP_TICKS);
        SetBits(Buffer, 1, 1);      /* the ack bit itself is not obfuscated */
        legic_prng_forward(1);      /* but it still occupies a bit period */
        return 1;
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
