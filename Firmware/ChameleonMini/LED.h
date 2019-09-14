/*
 * LED.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef _CM_LED_H_
#define _CM_LED_H_

#include <avr/io.h>

#define LED_HIGH_PORT        PORTA
#define LED_HIGH_MASK        (PIN5_bm | PIN4_bm | PIN3_bm | PIN2_bm)
#define LED_LOW_PORT         PORTE
#define LED_LOW_MASK         (PIN3_bm | PIN2_bm | PIN1_bm | PIN0_bm)
#define LED_LOW_PULSE_MASK   0
#define LED_HIGH_PULSE_MASK  0

typedef enum {
    LED_ONE,
    LED_TWO,
    LED_THREE,
    LED_FOUR,
    LED_FIVE,
    LED_SIX,
    LED_SEVEN,
    LED_EIGHT
} Led;

typedef struct {
    PORT_t * ledPort;
    uint8_t ledPin;
} LedPining_t;

void LEDInit(void);
void LEDTick(void);
void LEDSetOn(Led LedId);
void LEDSetOff(Led LedId);

#endif /* _CM_LED_H_ */
