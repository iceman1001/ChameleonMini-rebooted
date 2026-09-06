#include <avr/interrupt.h>
#include "Codec.h"

uint8_t CodecBuffer[CODEC_BUFFER_SIZE];

#ifdef CONFIG_LEGIC_PRIME_SUPPORT
/* The two vectors both codecs contend for; see Codec.h for why these are naked
 * trampolines. sbis skips the following instruction when the bit is set, and
 * skips a whole 2-word jmp, so a clear bit selects ISO14443-2A and a set bit
 * ISO14443-2F. The targets are signal functions issuing their own reti, so
 * nothing here may push, pop or clobber a register. */
ISR(CODEC_DEMOD_IN_INT0_VECT, ISR_NAKED) {
    __asm__ __volatile__ (
        "sbis %[gpio], %[bit]                   \n\t"
        "jmp  isr_ISO14443_2A_DEMOD_IN_INT0     \n\t"
        "jmp  isr_ISO14443_2F_DEMOD_IN_INT0     \n\t"
        :: [gpio] "I" (_SFR_IO_ADDR(CODEC_VECTOR_OWNER_GPIO)),
           [bit]  "I" (CODEC_VECTOR_OWNER_BIT)
    );
}

ISR(CODEC_TIMER_OVF_VECT, ISR_NAKED) {
    __asm__ __volatile__ (
        "sbis %[gpio], %[bit]                    \n\t"
        "jmp  isr_ISO14443_2A_TIMER_LOADMOD_OVF  \n\t"
        "jmp  isr_ISO14443_2F_TIMER_LOADMOD_OVF  \n\t"
        :: [gpio] "I" (_SFR_IO_ADDR(CODEC_VECTOR_OWNER_GPIO)),
           [bit]  "I" (CODEC_VECTOR_OWNER_BIT)
    );
}
#endif
