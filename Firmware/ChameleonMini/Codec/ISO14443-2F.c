/*
 * ISO14443-2F.c
 *
 * LEGIC Prime RF physical layer (card side), ChameleonMini-rebooted RevE port.
 *
 * Reader -> card: 100% ASK pulse-pause. A 20us pause followed by 40us of
 * carrier is a 0 (60us total), by 80us a 1 (100us total). Demodulated by
 * timing consecutive pause-start edges on the demod pin with a free-running
 * timer, rather than the fixed-rate sampling ISO14443-2A.c uses; the edges are
 * tens of microseconds apart and resolve comfortably at 4MHz.
 *
 * Card -> reader: OOK of the fc/64 (~212kHz) subcarrier, one bit per ~99us
 * slot, beginning 330us after the reader's last edge.
 *
 * Frames are handed to LegicPrimeAppProcess() still keystream-obfuscated; the
 * PRNG lives in the application layer.
 *
 * Reference: SAR-PR-2011-03 Sec. 3, and proxmark3 armsrc/legicrfsim.c.
 */

#include "ISO14443-2F.h"
#include "../System.h"
#include "../Application/Application.h"
#include "Codec.h"

/* fc/64 subcarrier, 50% duty via the OOK compare channel. */
#define LEGIC_SUBCARRIER_DIVIDER    64

/* Free-running stopwatch (TCD0, otherwise unused) for measuring reader
 * turnaround gaps, so the keystream can track the reader's real timing rather
 * than a fixed count. DIV64 of 32MHz = 2us/tick; gaps are < 1ms. */
#define CODEC_TIMER_GAP             TCD0
#define GAP_TICK_gc                 TC_CLKSEL_DIV64_gc
#define GAP_TICK_US                 2

/* Receive timebase: CODEC_TIMER_SAMPLING (TCC1) at DIV8 of F_CPU=32MHz
 * -> 4MHz, 0.25us/tick. A 0 bit measures 232-245 ticks, a 1 bit 393-408. */
#define RX_TICK_gc              TC_CLKSEL_DIV8_gc
#define RX_TIMEOUT_TICKS        500  /* > the 100us max bit: end of frame */
#define RX_MAX_BITS             24   /* legicrfsim.c RWD_MAX_FRAME_LEN is 23 */
#define RX_BIT_THRESHOLD        320  /* 80.2us, ~30% clear of both bit lengths */

/* Glitch filter: the shortest legitimate bit is 61.3us, so an edge arriving
 * sooner than this after the last accepted one is a demodulator bounce. It is
 * discarded without restarting the interval measurement, otherwise one bounce
 * splits a bit in two and the frame demodulates one bit too long. */
#define RX_MIN_BIT_TICKS        200     /* 50us */

/* Transmit timebase: CODEC_TIMER_LOADMOD (TCD1) is clocked from the reader's
 * carrier over event channel 6, as ISO14443-2A.c clocks its own load
 * modulation. System.c runs the 32MHz RC against the internal RC32K rather
 * than a crystal, so the CPU clock is only accurate to about a percent; over
 * the ~1.5ms of a 12-bit answer that accumulates to a third of a bit period
 * and the late bits fall outside the reader's sampling window. The carrier is
 * an exact reference and only the reader ever pauses it.
 *
 * Intervals are whole multiples of 64 carrier cycles because legicrfsim.c's
 * tick is one 212kHz subcarrier period:
 *   TAG_BIT_PERIOD   21 ticks =  1344 cycles =   99.1us
 *   TAG_FRAME_WAIT   70 ticks =  4480 cycles =  330.4us
 *   TAG_ACK_WAIT    758 ticks = 48512 cycles = 3577.0us
 *   RWD_TIME_PAUSE    4 ticks =   256 cycles =   18.9us */
#define TX_TICK_gc              TC_CLKSEL_EVCH6_gc
#define TAG_FRAME_WAIT_CYCLES   4480
#define TAG_ACK_WAIT_CYCLES     48512
#define RWD_PAUSE_CYCLES        271     /* 20.0us, ISO14443 Annex F */
#define TX_BIT_CYCLES           1344    /* 99.1us; 100us drifts late bits off the reader's grid */

/* legicrfsim.c measures both waits from the reader's frame end, i.e. the last
 * pause edge plus the pause itself, while the RX-end interrupt here fires
 * RX_TIMEOUT_TICKS after that edge. ISO14443FCodecTask additionally subtracts
 * its own scheduling latency. Both corrections arrive in RX ticks: one tick is
 * 3.39 carrier cycles, i.e. 217/64 to within 0.02%. Evaluated once per frame. */
#define RX_TICKS_TO_CARRIER(t)  ((uint16_t)(((uint32_t)(t) * 217UL) >> 6))

#define TX_GAP_CYCLES       (TAG_FRAME_WAIT_CYCLES + RWD_PAUSE_CYCLES \
                             - RX_TICKS_TO_CARRIER(RX_TIMEOUT_TICKS))

/* Transmit shaping, in carrier cycles. Determined by sweeping each against a
 * proxmark3 and reading back per-bit demodulated power.
 *
 * LEAD starts bit 0 early: a tag frame has no start bit, so bit 0 follows the
 * 330us quiet gap and its modulation is still ramping when the reader samples
 * ~36us in. It is taken out of the gap and added to bit 0's own period, so no
 * later bit boundary moves.
 *
 * WARM is a pre-charge pilot in the gap tail, modulated only when bit 0 is a 1.
 * The window is consumed either way, which is what keeps bit 0 on the grid;
 * modulating it unconditionally floods the following bits to a uniform level.
 *
 * DAMP ends each bit early so that a 0 following a 1 has time for the tank to
 * ring down. Without it the reader only reads the card with its threshold
 * raised (hw sethfthresh -l 40). */
#define TX_LEAD_CYCLES      200     /* ~14.8us */
#define TX_WARM_CYCLES      271     /* ~20us   */
#define TX_DAMP_CYCLES      407     /* ~30us   */

#define TX_ACK_GAP_CYCLES   (TAG_ACK_WAIT_CYCLES + RWD_PAUSE_CYCLES \
                             - RX_TICKS_TO_CARRIER(RX_TIMEOUT_TICKS))

typedef enum {
    TX_GAP,
    TX_WARM_ON,
    TX_BIT,
    TX_GUARD,
    TX_DONE
} TxStateType;

/* Pause-to-pause intervals of the frame being demodulated, in RX ticks;
 * thresholded into bits once the frame is complete. */
static volatile uint16_t RxIntervals[RX_MAX_BITS];
volatile uint16_t BitCount;
static volatile uint16_t BitSent;
static volatile bool FirstEdge;
static volatile TxStateType TxState;
static volatile bool TxReady;

/* Turnaround gap measurement. GapAnchor holds the end of the previous frame
 * (our TX_DONE, or a reader frame's last edge); LegicRxGapMicros is the gap
 * before the frame currently being processed, read by LegicPrime.c. */
static volatile uint16_t GapAnchor;
static volatile uint16_t LastEdgeStamp;
volatile uint16_t LegicRxGapMicros;
volatile uint16_t LegicFrameSeq;      /* ++ at each frame's first edge */
volatile uint16_t LegicGapRing[8];    /* per-frame gaps, indexed by LegicFrameSeq */

static void SetBit(uint8_t *buf, uint16_t pos, bool v) {
    if (v) {
        buf[pos / 8] |= (1 << (pos % 8));
    } else {
        buf[pos / 8] &= ~(1 << (pos % 8));
    }
}

static bool GetBit(const uint8_t *buf, uint16_t pos) {
    return (buf[pos / 8] & (1 << (pos % 8))) != 0;
}

static void Initialize(void) {
    /* CARRIER input, routed to EVSYS channel 6 (used to clock the
     * subcarrier timer while it's gated on). */
    CODEC_CARRIER_IN_PORT.DIRCLR = CODEC_CARRIER_IN_MASK;
    CODEC_CARRIER_IN_PORT.CODEC_CARRIER_IN_PINCTRL = PORT_ISC_BOTHEDGES_gc;
    EVSYS.CH6MUX = CODEC_CARRIER_IN_EVMUX;

    /* Demod pin: rising edge = start of reader's pause. */
    CODEC_DEMOD_IN_PORT.DIRCLR = CODEC_DEMOD_IN_MASK;
    CODEC_DEMOD_IN_PORT.CODEC_DEMOD_IN_PINCTRL0 = PORT_ISC_RISING_gc;
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
    CODEC_DEMOD_IN_PORT.INTCTRL = PORT_INT0LVL_HI_gc;

    /* Load modulation output. This is the pin that switches the modulator;
     * CODEC_SUBCARRIER_PORT only supplies the frequency. */
    CODEC_LOADMOD_PORT.DIRSET = CODEC_LOADMOD_MASK;
    CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;

    /* Subcarrier output, OOK, off by default. */
    CODEC_SUBCARRIER_PORT.DIRSET = CODEC_SUBCARRIER_MASK;
    CODEC_SUBCARRIER_PORT.OUTCLR = CODEC_SUBCARRIER_MASK;
    CODEC_SUBCARRIER_TIMER.PER = LEGIC_SUBCARRIER_DIVIDER - 1;
    CODEC_SUBCARRIER_TIMER.CODEC_SUBCARRIER_CC_OOK = LEGIC_SUBCARRIER_DIVIDER / 2;
    CODEC_SUBCARRIER_TIMER.CTRLB = CODEC_SUBCARRIER_CCEN_OOK | TC_WGMODE_SINGLESLOPE_gc;
    /* Free-run the subcarrier for the whole session; the load-modulation pin
     * gates what reaches the antenna. Stopping a single-slope PWM freezes its
     * compare output at whatever level the count had reached, which loses the
     * first modulated bit of the following frame. */
    CODEC_SUBCARRIER_TIMER.CNT = 0;
    CODEC_SUBCARRIER_TIMER.CTRLA = TC_CLKSEL_EVCH6_gc; /* free-run */

    /* Turnaround-gap stopwatch: free-run for the whole session. */
    CODEC_TIMER_GAP.PER = 0xFFFF;
    CODEC_TIMER_GAP.CNT = 0;
    CODEC_TIMER_GAP.CTRLA = GAP_TICK_gc;
    GapAnchor = 0;
    LastEdgeStamp = 0;
    LegicRxGapMicros = 0;
}

static void StartDemod(void) {
    CodecSetDemodPower(true);

    BitCount = 0;
    FirstEdge = true;

    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_OFF_gc;

    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm;
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

void isr_ISO14443_2F_DEMOD_IN_INT0(void) {
    if (FirstEdge) {
        /* Start of a new frame: arm the stopwatch, don't classify a bit yet
         * (need a second edge to measure the first bit's duration). */
        FirstEdge = false;
        CODEC_TIMER_SAMPLING.CNT = 0;
        CODEC_TIMER_SAMPLING.PER = RX_TIMEOUT_TICKS - 1;
        CODEC_TIMER_SAMPLING.INTFLAGS = TC0_OVFIF_bm;
        CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_HI_gc;
        CODEC_TIMER_SAMPLING.CTRLA = RX_TICK_gc;
        /* Bit timer is armed; now latch the turnaround gap (does not affect
         * bit timing). */
        uint16_t gnow = CODEC_TIMER_GAP.CNT;
        LegicRxGapMicros = (uint16_t)((gnow - GapAnchor) * GAP_TICK_US);
        LastEdgeStamp = gnow;
        LegicFrameSeq++;
        LegicGapRing[LegicFrameSeq & 7] = LegicRxGapMicros;
        return;
    }

    uint16_t elapsed = CODEC_TIMER_SAMPLING.CNT;

    if (elapsed < RX_MIN_BIT_TICKS) {
        /* Bounce, not a bit boundary. Ignore it and deliberately leave the
         * stopwatch running so the next real edge still measures the whole
         * bit from the last accepted one. */
        return;
    }
    CODEC_TIMER_SAMPLING.CNT = 0;
    LastEdgeStamp = CODEC_TIMER_GAP.CNT;

    /* Record the raw interval; bits are classified once the frame is in. */
    if (BitCount < RX_MAX_BITS) {
        RxIntervals[BitCount] = elapsed;
        BitCount++;
    }
}

/* Turn the frame's buffered intervals into bits and build the answer.
 *
 * Runs from the RX-end interrupt rather than the main loop: the reader opens
 * the response slot a fixed 330us after frame end, which main-loop scheduling
 * cannot reliably meet. Costs ~40us of bounded interrupt time, during which
 * the demod edge interrupt is masked and the load-modulation timer will not
 * fire for another 224us. */
static void BuildResponse(void) {
    uint16_t DemodBitCount = BitCount;

    for (uint16_t i = 0; i < DemodBitCount; i++) {
        SetBit(CodecBuffer, i, RxIntervals[i] > RX_BIT_THRESHOLD);
    }

    uint16_t AnswerBitCount = ApplicationProcess(CodecBuffer, DemodBitCount);

    if (AnswerBitCount != LEGICPRIME_APP_NO_RESPONSE) {
        BitCount = AnswerBitCount;
        BitSent = 0;
        /* A one-bit answer is the write ACK: 3.57ms, not 330us. The slot is
         * already counting, so just stretch it. */
        if (AnswerBitCount == 1) {
            CODEC_TIMER_LOADMOD.PER = TX_ACK_GAP_CYCLES - 1;
        }
        TxReady = true;
    } else {
        /* Nothing to say: cancel the slot and listen again. */
        CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
        CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
        StartDemod();
    }
}

ISR(CODEC_TIMER_SAMPLING_OVF_VECT) {
    /* No further edge within the timeout: the frame is over. */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

    /* Anchor the next turnaround at this frame's end; TX_DONE overrides it if
     * we answer. */
    GapAnchor = LastEdgeStamp;

    if (BitCount > 0) {
        CodecSetDemodPower(false);
        /* Arm the response slot before building the answer. This instant is a
         * fixed offset from the reader's last pause edge, so the gap is exact
         * regardless of how long BuildResponse then takes. */
        TxReady = false;
        TxState = TX_GAP;
        /* Re-arm with a fresh CNT=0 and an explicit start: an event-clocked
         * timer that is already running does not reliably overflow on a bare
         * PER rewrite. */
        CODEC_TIMER_LOADMOD.CNT = 0;
        CODEC_TIMER_LOADMOD.PER = TX_GAP_CYCLES - TX_LEAD_CYCLES - TX_WARM_CYCLES - 1;
        CODEC_TIMER_LOADMOD.INTFLAGS = TC1_OVFIF_bm;
        CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_HI_gc;
        CODEC_TIMER_LOADMOD.CTRLA = TX_TICK_gc;

        BuildResponse();
    } else {
        StartDemod();
    }
}

void isr_ISO14443_2F_TIMER_LOADMOD_OVF(void) {
    switch (TxState) {
        case TX_GAP:
            if (!TxReady) {
                /* Answer not ready yet: stay silent rather than send garbage. */
                CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
                CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
                StartDemod();   /* restart RX immediately, not via the main loop
                                 * (so ApplicationTask latency can't delay it) */
                break;
            }
            /* Pre-charge window; see TX_WARM_CYCLES. The RX-end ISR already
             * shortened the gap by it, so it is consumed either way. */
            CODEC_SUBCARRIER_TIMER.CNT = 0;
            if (GetBit(CodecBuffer, 0)) {
                CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
            } else {
                CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
            }
            CODEC_TIMER_LOADMOD.PER = TX_WARM_CYCLES - 1;
            TxState = TX_WARM_ON;
            break;

        case TX_WARM_ON:
            /* The pilot runs straight into bit 0 with no quiet gap and no
             * subcarrier reset, so bit 0 starts from a warmed, oscillating
             * tank and behaves like a mid-frame bit. */
            TxState = TX_BIT;
            CODEC_TIMER_LOADMOD.PER = TX_BIT_CYCLES + TX_LEAD_CYCLES - 1;
            goto drive_bit;

        case TX_BIT:
        drive_bit: {
            uint8_t bv = GetBit(CodecBuffer, BitSent);
            if (bv) {
                CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
            } else {
                CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
            }
            uint16_t period = TX_BIT_CYCLES + ((BitSent == 0) ? TX_LEAD_CYCLES : 0);
            /* Drive the bit for the head of the slot, then a trailing OFF
             * guard so a following 0 reads clean. Advance after the guard. */
            CODEC_TIMER_LOADMOD.PER = (period - TX_DAMP_CYCLES) - 1;
            TxState = TX_GUARD;
            break;
        }

        case TX_GUARD: {
            /* Trailing gap: modulation off, let the tank ring decay. */
            CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
            CODEC_TIMER_LOADMOD.PER = TX_DAMP_CYCLES - 1;
            BitSent++;
            TxState = (BitSent >= BitCount) ? TX_DONE : TX_BIT;
            break;
        }

        case TX_DONE:
        default:
            CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
            CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
            GapAnchor = CODEC_TIMER_GAP.CNT;   /* our answer end: anchor next gap */
            StartDemod();   /* immediate RX restart from the ISR */
            break;
    }
}

void ISO14443FCodecInit(void) {
    /* See Codec.h. */
    CodecClaimSharedVectorsF();

    Initialize();
    StartDemod();
}

void ISO14443FCodecTask(void) {
    /* Nothing to do: frames are demodulated, answered and the receiver
     * restarted entirely in interrupt context. Kept because Configuration.c
     * requires a CodecTaskFunc for every configuration. */
}

