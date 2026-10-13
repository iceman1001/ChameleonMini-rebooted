#ifndef CODEC_H_
#define CODEC_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "../Common.h"
#include "../Configuration.h"

#include "ISO14443-2A.h"
#ifdef CONFIG_LEGIC_PRIME_SUPPORT
#include "ISO14443-2F.h"
#endif

#ifdef CONFIG_LEGIC_PRIME_SUPPORT
/* Both codecs are compiled in -- Mifare/NTAG use ISO14443-2A, LEGIC Prime uses
 * ISO14443-2F -- but the hardware has one demod/loadmod circuit, so they
 * contend for the demod edge interrupt and the load-modulation timer overflow.
 * (Their sampling vectors differ, CCA against OVF, and stay attached directly.)
 * Only one codec is ever active, but an ISR() in both files is a duplicate
 * symbol.
 *
 * The contended vectors fire every half bit, ~4.7us at 106kbit/s, so dispatch
 * has to be nearly free. A C function pointer is not: the compiler cannot see
 * the callee and spills every call-clobbered register, 15 push/pop pairs and
 * about 85 cycles, which exceeds half the sampling budget. Each contended
 * vector is therefore a naked trampoline in Codec.c that tests one bit of
 * GPIOR0 -- low I/O space, so sbis needs no register -- and jumps into the
 * owning codec's handler. Cost is sbis + jmp and no registers touched.
 *
 * Without LEGIC the contention does not exist, and ISO14443-2A attaches to the
 * vectors directly with a plain ISR() exactly as it always has. */
#define CODEC_VECTOR_OWNER_GPIO     GPIOR0
#define CODEC_VECTOR_OWNER_BIT      0   /* 0 = ISO14443-2A, 1 = ISO14443-2F */

/* Handlers reached from the trampolines. Referenced by name from inline asm,
 * so they must stay externally visible and must not be inlined away. */
#define CODEC_ISR_HANDLER   __attribute__((signal, used, externally_visible))

void isr_ISO14443_2A_DEMOD_IN_INT0(void) CODEC_ISR_HANDLER;
void isr_ISO14443_2A_TIMER_LOADMOD_OVF(void) CODEC_ISR_HANDLER;
void isr_ISO14443_2F_DEMOD_IN_INT0(void) CODEC_ISR_HANDLER;
void isr_ISO14443_2F_TIMER_LOADMOD_OVF(void) CODEC_ISR_HANDLER;

#define CODEC_2A_DEMOD_IN_INT0_ISR      void isr_ISO14443_2A_DEMOD_IN_INT0(void)
#define CODEC_2A_TIMER_LOADMOD_OVF_ISR  void isr_ISO14443_2A_TIMER_LOADMOD_OVF(void)
#else
#define CODEC_2A_DEMOD_IN_INT0_ISR      ISR(CODEC_DEMOD_IN_INT0_VECT)
#define CODEC_2A_TIMER_LOADMOD_OVF_ISR  ISR(CODEC_TIMER_OVF_VECT)
#endif

#define CODEC_DEMOD_POWER_PORT      PORTB
#define CODEC_DEMOD_POWER_MASK      PIN1_bm
#define CODEC_DEMOD_IN_PORT         PORTB
#define CODEC_DEMOD_IN_MASK         (CODEC_DEMOD_IN_MASK0 | CODEC_DEMOD_IN_MASK1)
#define CODEC_DEMOD_IN_MASK0        PIN0_bm
#define CODEC_DEMOD_IN_MASK1        PIN2_bm
#define CODEC_DEMOD_IN_PINCTRL0     PIN0CTRL
#define CODEC_DEMOD_IN_PINCTRL1     PIN2CTRL
#define CODEC_DEMOD_IN_EVMUX0       EVSYS_CHMUX_PORTB_PIN0_gc
#define CODEC_DEMOD_IN_EVMUX1       EVSYS_CHMUX_PORTB_PIN2_gc
#define CODEC_DEMOD_IN_INT0_VECT	PORTB_INT0_vect
#define CODEC_LOADMOD_PORT			PORTC
#define CODEC_LOADMOD_MASK			PIN6_bm
#define CODEC_CARRIER_IN_PORT		PORTC
#define CODEC_CARRIER_IN_MASK		PIN2_bm
#define CODEC_CARRIER_IN_PINCTRL 	PIN2CTRL
#define CODEC_CARRIER_IN_EVMUX		EVSYS_CHMUX_PORTC_PIN2_gc
#define CODEC_SUBCARRIER_PORT		PORTC
#define CODEC_SUBCARRIER_MASK_PSK	PIN0_bm
#define CODEC_SUBCARRIER_MASK_OOK	PIN1_bm
#define CODEC_SUBCARRIER_MASK		(CODEC_SUBCARRIER_MASK_PSK | CODEC_SUBCARRIER_MASK_OOK)
#define CODEC_SUBCARRIER_TIMER		TCC0
#define CODEC_SUBCARRIER_CC_PSK		CCA
#define CODEC_SUBCARRIER_CC_OOK		CCB
#define CODEC_SUBCARRIER_CCEN_PSK	TC0_CCAEN_bm
#define CODEC_SUBCARRIER_CCEN_OOK	TC0_CCBEN_bm
#define CODEC_TIMER_SAMPLING		TCC1
#define CODEC_TIMER_SAMPLING_CCA_VECT	TCC1_CCA_vect
#define CODEC_TIMER_SAMPLING_OVF_VECT	TCC1_OVF_vect
#define CODEC_TIMER_LOADMOD       	TCD1
#define CODEC_TIMER_OVF_VECT		TCD1_OVF_vect

#define CODEC_BUFFER_SIZE           256 /* Byte */

#define CODEC_CARRIER_FREQ          13560000

extern uint8_t CodecBuffer[CODEC_BUFFER_SIZE];

INLINE void CodecInit(void) {
    ActiveConfiguration.CodecInitFunc();
}

INLINE void CodecTask(void) {
    ActiveConfiguration.CodecTaskFunc();
}

#ifdef CONFIG_LEGIC_PRIME_SUPPORT
/* Point the contended vectors at this codec, before touching hardware that
 * could fire them. GPIOR0 resets to 0, so ISO14443-2A owns them out of reset. */
INLINE void CodecClaimSharedVectorsA(void) {
    CODEC_VECTOR_OWNER_GPIO &= ~_BV(CODEC_VECTOR_OWNER_BIT);
}

INLINE void CodecClaimSharedVectorsF(void) {
    CODEC_VECTOR_OWNER_GPIO |= _BV(CODEC_VECTOR_OWNER_BIT);
}
#else
/* No contention without LEGIC: ISO14443-2A owns the vectors outright. */
#define CodecClaimSharedVectorsA()  do { } while (0)
#endif

INLINE void CodecSetDemodPower(bool bOnOff) {
    CODEC_DEMOD_POWER_PORT.DIRSET = CODEC_DEMOD_POWER_MASK;

    if (bOnOff) {
        CODEC_DEMOD_POWER_PORT.OUTSET = CODEC_DEMOD_POWER_MASK;
    } else {
        CODEC_DEMOD_POWER_PORT.OUTCLR = CODEC_DEMOD_POWER_MASK;
    }
}

#endif /* CODEC_H_ */
