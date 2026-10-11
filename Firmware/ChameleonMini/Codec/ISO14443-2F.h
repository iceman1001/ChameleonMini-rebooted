/*
 * ISO14443-2F.h
 *
 * LEGIC Prime RF physical layer (card side), ChameleonMini-rebooted RevE port.
 */

#ifndef ISO14443_2F_H_
#define ISO14443_2F_H_

#include <stdint.h>
#include <stdbool.h>

/* Which side sent the frame before the one currently being handed to
 * LegicPrimeAppProcess(): true if it was our own answer, false if it was the
 * reader. The keystream ticks a turnaround costs depend on this and not on how
 * long the turnaround took -- a proxmark3 trace shows both transitions taking
 * the same 220us while the schedule needs 3 ticks after our frame and 2
 * between two reader frames. The constants live in LegicPrime.c.
 *
 * Valid only for the duration of the ApplicationProcess() call. */
extern volatile bool LegicGapAfterOurFrame;

/* Measured length of that gap in bit periods, for staleness detection only --
 * not a tick count. LEGIC_GAP_DESYNC means the reader paused so long that the
 * running session cannot be assumed intact any more. */
#define LEGIC_GAP_DESYNC    0xFF
extern volatile uint8_t LegicRxGapPeriods;

#ifdef LEGIC_GAP_DEBUG
/* Ring log of the last LEGIC_GAP_LOG_SIZE received frames, for checking the
 * turnaround measurement against reality -- see the LEGICGAPS command.
 *
 * A ring rather than a first-N log because the interesting exchange is rarely
 * the first one after power-up: `hf search` probes several protocols first,
 * and a first-N log fills with that noise. Each entry carries the frame it
 * belongs to, so an entry can be attributed instead of guessed at:
 *   Log  = the gap before that frame, in RX ticks (0.25us)
 *   Info = frame bit count in bits 0..5, the after-our-frame bias in bit 7 */
#define LEGIC_GAP_LOG_SIZE  10
#define LEGIC_GAP_LOG_BIAS  0x80
#define LEGIC_GAP_LOG_BITS  0x3F
extern volatile uint16_t LegicGapLog[LEGIC_GAP_LOG_SIZE];
extern volatile uint8_t  LegicGapLogInfo[LEGIC_GAP_LOG_SIZE];
extern volatile uint8_t  LegicGapLogNext;
extern volatile uint8_t  LegicGapLogCount;
#endif

void ISO14443FCodecInit(void);
void ISO14443FCodecTask(void);

#endif /* ISO14443_2F_H_ */
