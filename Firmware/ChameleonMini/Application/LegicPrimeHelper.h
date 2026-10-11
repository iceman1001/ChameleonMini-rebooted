/*
 * LegicPrimeHelper.h
 * This is part of the Chameleon project
 * https://github.com/iceman1001/ChameleonMini-rebooted
*/

#ifndef __LEGIC_PRIME_HELPER_H__
#define __LEGIC_PRIME_HELPER_H__

extern uint8_t legic_prng_get_bit(void);
extern void legic_prng_forward(uint8_t count);
extern uint32_t legic_xor_bits(uint32_t value, uint8_t n);

#endif /* __LEGIC_PRIME_HELPER_H__ */