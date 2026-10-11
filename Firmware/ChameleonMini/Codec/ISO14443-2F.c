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

/* ---- Inter-frame gap measurement (see ISO14443-2F.h) ----
 *
 * One bit period is TAG_BIT_PERIOD = 1344 carrier cycles = 99.115us, which at
 * 0.25us/tick is 396.5 RX ticks.
 *
 * The keystream does advance with elapsed time across a turnaround, so this
 * has to be measured rather than assumed. Truncated, not rounded, and with no
 * offset here -- LegicPrime.c adds the one-tick bias that applies after our
 * own frames. Derived from two hf sniff captures, one against a proxmark3 and
 * one against a real LEGIC lock, six turnarounds in total, all six fitting:
 *
 *   turnaround                     gap us   floor  bias  ticks
 *   pm3   after our type frame       220.0      2     1      3
 *   pm3   ack -> command             220.0      2     0      2
 *   pm3   after our read answer      220.0      2     1      3
 *   lock  after our type frame       221.9      2     1      3
 *   lock  ack -> command             355.2      3     0      3
 *   lock  after our read answer      523.5      5     1      6
 *
 * proxmark3 paces every turnaround at 220us, which is why its trace alone
 * looks like fixed counts of 2 and 3 and cannot tell the two models apart.
 * The lock's 355us and 523us are what separate them.
 *
 * Truncating against 396 rather than 396.5 overstates the quotient by 0.12%,
 * which cannot reach a whole period below ~800 of them. The tightest margin to
 * a floor() boundary in the data above is 21.8us, against a measurement good
 * to a few microseconds. */
#define RX_TICKS_PER_BIT_PERIOD 396

/* Generous on purpose -- a false positive drops a working session, while a
 * false negative only delays the reader's own retry by one frame. */
#define LEGIC_GAP_STALE_PERIODS 96

/* The RX-end interrupt runs RX_TIMEOUT_TICKS after the reader's last pause
 * edge, but the frame itself ended one pause earlier than that -- the same
 * correction TX_GAP_CYCLES makes, in the other direction. */
#define CARRIER_TO_RX_TICKS(c)  ((uint16_t)(((uint32_t)(c) * 64UL) / 217UL))
#define RX_END_LATENCY_TICKS    (RX_TIMEOUT_TICKS - CARRIER_TO_RX_TICKS(RWD_PAUSE_CYCLES))

/* Sentinel origin: there is no meaningful previous frame to measure from. */
#define GAP_ORIGIN_INVALID      0xFFFF

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

/* Gap stopwatch: the raw RX-tick reading at this frame's first edge and the
 * length it converts to, plus which side sent the frame before this one. */
static volatile uint16_t RxGapRawTicks;
static volatile bool     GapValid;
/* Who sent the frame before the *next* one. Latched into
 * LegicGapAfterOurFrame at that frame's first edge, never written later:
 * GapRestart() runs before BuildResponse(), so writing the exported flag
 * directly would clobber it while ApplicationProcess() still needs it for the
 * frame in hand -- which silently dropped the one-tick bias. */
static volatile bool     GapPendingAfterOurFrame;
volatile uint8_t         LegicRxGapPeriods;
volatile bool            LegicGapAfterOurFrame;

#ifdef LEGIC_GAP_DEBUG
volatile uint16_t LegicGapLog[LEGIC_GAP_LOG_SIZE];
volatile uint8_t  LegicGapLogInfo[LEGIC_GAP_LOG_SIZE];
volatile uint8_t  LegicGapLogNext;
volatile uint8_t  LegicGapLogCount;
#endif

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

}

/* Re-origin the gap stopwatch. Origin is how much time has already passed
 * since the previous frame ended, in RX ticks, so that CODEC_TIMER_SAMPLING
 * reads the whole turnaround when the next frame's first edge arrives.
 * GAP_ORIGIN_INVALID means there is nothing meaningful to measure from.
 * AfterOurFrame records who sent that previous frame; the application needs it
 * for the turnaround's one-tick bias.
 *
 * Kept separate from StartDemod() because it has to run at a *fixed* offset
 * from the frame end. StartDemod() runs after BuildResponse(), and those ~27us
 * of keystream work would otherwise be subtracted from the measurement -- with
 * a proxmark3's 220us turnaround sitting only 21.8us above a floor() boundary,
 * that lost one whole tick and desynced on the first read command. A lock
 * pacing at 355us happened to survive it, which is why the two readers
 * disagreed. */
static void GapRestart(uint16_t Origin, bool AfterOurFrame) {
    GapPendingAfterOurFrame = AfterOurFrame;
    GapValid = (Origin != GAP_ORIGIN_INVALID);

    /* PER is opened right up so a long reader pause cannot wrap it -- 0xFFFF
     * ticks is 16.4ms, well past LEGIC_GAP_STALE_PERIODS. The overflow
     * interrupt stays off; while a frame is being received PER is narrowed
     * back to RX_TIMEOUT_TICKS and the overflow regains its end-of-frame
     * meaning. */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.PER = 0xFFFF;
    CODEC_TIMER_SAMPLING.CNT = GapValid ? Origin : 0;
    CODEC_TIMER_SAMPLING.CTRLA = RX_TICK_gc;
}

/* Arm the receiver. Deliberately does not touch the gap stopwatch. */
static void StartDemod(void) {
    CodecSetDemodPower(true);

    BitCount = 0;
    FirstEdge = true;

    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm;
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

/* Raw stopwatch reading -> whole bit periods elapsed, truncated. */
static uint8_t GapToBitPeriods(uint16_t RawTicks) {
    uint16_t Periods;

    if (!GapValid) {
        return LEGIC_GAP_DESYNC;
    }
    Periods = RawTicks / RX_TICKS_PER_BIT_PERIOD;
    if (Periods > LEGIC_GAP_STALE_PERIODS) {
        return LEGIC_GAP_DESYNC;
    }
    return (uint8_t)Periods;
}

void isr_ISO14443_2F_DEMOD_IN_INT0(void) {
    if (FirstEdge) {
        /* Start of a new frame: arm the stopwatch, don't classify a bit yet
         * (need a second edge to measure the first bit's duration). */
        FirstEdge = false;
        /* Read and convert the gap before restarting the timer for bit
         * measurement. Done here rather than at frame end only to keep the
         * division (~250 cycles, well inside the 61us to the next edge) off
         * the response deadline. */
        RxGapRawTicks = CODEC_TIMER_SAMPLING.CNT;
        LegicRxGapPeriods = GapToBitPeriods(RxGapRawTicks);
        LegicGapAfterOurFrame = GapPendingAfterOurFrame;
        CODEC_TIMER_SAMPLING.CNT = 0;
        CODEC_TIMER_SAMPLING.PER = RX_TIMEOUT_TICKS - 1;
        CODEC_TIMER_SAMPLING.INTFLAGS = TC0_OVFIF_bm;
        CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_HI_gc;
        CODEC_TIMER_SAMPLING.CTRLA = RX_TICK_gc;
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
 * cannot reliably meet. The demod edge interrupt is masked throughout and the
 * load-modulation timer will not fire for another 190us (TX_GAP_CYCLES less
 * the two pre-charge windows, already armed above).
 *
 * The application's share of that is bounded but no longer small: it has to
 * de-obfuscate the command, look up the addressed byte and obfuscate the
 * answer here, because the reader picks the address and the gap it just took
 * is only knowable after the fact -- neither can be precomputed. Worst case is
 * roughly 140us of the 190us, with LEGIC_GAP_MAX_TICKS setting the bound. */
static void BuildResponse(void) {
    uint16_t DemodBitCount = BitCount;
    uint8_t *Out = CodecBuffer;
    uint8_t Mask = 1;

    /* Walking pointer and rotating mask rather than indexing by bit position:
     * see the note on legic_xor_bits() in LegicPrime.c. */
    for (uint16_t i = 0; i < DemodBitCount; i++) {
        if (RxIntervals[i] > RX_BIT_THRESHOLD) {
            *Out |= Mask;
        } else {
            *Out &= (uint8_t)~Mask;
        }
        if ((Mask <<= 1) == 0) {
            Mask = 1;
            Out++;
        }
    }

    uint16_t AnswerBitCount = ApplicationProcess(CodecBuffer, DemodBitCount);

    if (AnswerBitCount != LEGICPRIME_APP_NO_RESPONSE) {
        BitCount = AnswerBitCount;
        BitSent = 0;
        /* A one-bit answer is the write ACK: 3.57ms, not 330us. The slot is
         * already counting, so just stretch it -- taking the pre-charge
         * windows out of it the same way the RX-end ISR did for the 330us
         * slot, or the ack lands 471 cycles (35us) late. */
        if (AnswerBitCount == 1) {
            CODEC_TIMER_LOADMOD.PER = TX_ACK_GAP_CYCLES - TX_LEAD_CYCLES
                                      - TX_WARM_CYCLES - 1;
        }
        TxReady = true;
    } else {
        /* Nothing to say: cancel the slot and listen again. The gap to the
         * reader's next frame runs from the end of the frame just received. */
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

    if (BitCount > 0) {
        /* Fixed offset from the reader's frame end -- must happen before
         * BuildResponse(); see GapRestart(). */
        GapRestart(RX_END_LATENCY_TICKS, false);
        CodecSetDemodPower(false);
#ifdef LEGIC_GAP_DEBUG
        /* GapRestart() above only touched the pending flag, so
         * LegicGapAfterOurFrame still describes this frame. */
        LegicGapLog[LegicGapLogNext] = RxGapRawTicks;
        LegicGapLogInfo[LegicGapLogNext] =
            (uint8_t)(BitCount & LEGIC_GAP_LOG_BITS)
            | (LegicGapAfterOurFrame ? LEGIC_GAP_LOG_BIAS : 0);
        if (++LegicGapLogNext >= LEGIC_GAP_LOG_SIZE) {
            LegicGapLogNext = 0;
        }
        if (LegicGapLogCount < LEGIC_GAP_LOG_SIZE) {
            LegicGapLogCount++;
        }
#endif
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
        /* A lone edge with no second one: not a frame. Carry the stopwatch
         * across it rather than losing the session -- the glitch happened
         * RxGapRawTicks into the gap and the timeout has run since. */
        GapRestart(GapValid ? (uint16_t)(RxGapRawTicks + RX_TIMEOUT_TICKS)
                            : GAP_ORIGIN_INVALID,
                   GapPendingAfterOurFrame);
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
                /* Restart RX immediately, not via the main loop (so
                 * ApplicationTask latency can't delay it). We owed the reader
                 * an answer and stayed silent, so the keystream is beyond
                 * recovery: force the application to wait for a fresh RAND. */
                GapRestart(GAP_ORIGIN_INVALID, false);
                StartDemod();
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
            /* Immediate RX restart from the ISR. This instant is the end of
             * our frame's last bit slot, so the gap to the reader's next frame
             * starts here: origin 0. */
            GapRestart(0, true);
            StartDemod();
            break;
    }
}

void ISO14443FCodecInit(void) {
    /* See Codec.h. */
    CodecClaimSharedVectorsF();

    Initialize();
    GapRestart(GAP_ORIGIN_INVALID, false);
    StartDemod();
}

void ISO14443FCodecTask(void) {
    /* Nothing to do: frames are demodulated, answered and the receiver
     * restarted entirely in interrupt context. Kept because Configuration.c
     * requires a CodecTaskFunc for every configuration. */
}

