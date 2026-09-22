/*
 * ISO14443-2F.h
 *
 * LEGIC Prime RF physical layer (card side), ChameleonMini-rebooted RevE port.
 */

#ifndef ISO14443_2F_H_
#define ISO14443_2F_H_

#include <stdint.h>

/* Reader turnaround (us) before the frame currently being processed, and a
 * counter that increments at each frame's first edge. */
extern volatile uint16_t LegicRxGapMicros;
extern volatile uint16_t LegicFrameSeq;
extern volatile uint16_t LegicGapRing[8];

void ISO14443FCodecInit(void);
void ISO14443FCodecTask(void);

#endif /* ISO14443_2F_H_ */
