/*
 * LED.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef LED_H
#define LED_H

#include "Common.h"
#include <avr/io.h>

#define LED_HEIGH_PORT  PORTA
#define LED_ONE	        PIN5_bm
#define LED_TWO		    PIN4_bm
#define LED_THREE		PIN3_bm
#define LED_FOUR		PIN2_bm
#define LED_HEIGH_MASK	(LED_ONE | LED_TWO | LED_THREE| LED_FOUR)

#define LED_LOW_PORT    PORTE
#define LED_FIVE	    PIN3_bm
#define LED_SIX		    PIN2_bm
#define LED_SEVEN		PIN1_bm
#define LED_EIGHT		PIN0_bm
#define LED_LOW_MASK	(LED_FIVE | LED_SIX | LED_SEVEN| LED_EIGHT)
			
extern uint8_t LEDHighPulseMask, LEDLowPulseMask;

static inline
void LEDInit(void) {
	LED_LOW_PORT.DIRSET = LED_LOW_MASK;
	LED_HEIGH_PORT.DIRSET = LED_HEIGH_MASK;
}

static inline
void LEDLowSetOn(uint8_t Mask) {
	LED_HEIGH_PORT.OUTSET = Mask;
}

static inline
void LEDLowSetOff(uint8_t Mask) {
	LED_HEIGH_PORT.OUTCLR = Mask;
}

static inline
void LEDHighSetOn(uint8_t Mask) {
	LED_LOW_PORT.OUTSET = Mask;
}

static inline
void LEDHighSetOff(uint8_t Mask) {
	LED_LOW_PORT.OUTCLR = Mask;
}

static inline
void LEDLowToggle(uint8_t Mask) {
	LED_HEIGH_PORT.OUTTGL = Mask;
}

static inline
void LEDHighToggle(uint8_t Mask) {
	LED_LOW_PORT.OUTTGL = Mask;
}

static inline
void LEDLowPulse(uint8_t Mask) {
	LEDLowPulseMask = Mask;
	LED_HEIGH_PORT.OUTSET = Mask;
}

static inline
void LEDHighPulse(uint8_t Mask) {
	LEDHighPulseMask = Mask;
	LED_LOW_PORT.OUTSET = Mask;
}

static inline
void LEDTick(void) {
	LED_HEIGH_PORT.OUTCLR = LEDLowPulseMask;
	LEDLowPulseMask = 0;
	LED_LOW_PORT.OUTCLR = LEDHighPulseMask;
	LEDHighPulseMask = 0;
}

static inline
void CARDInit(void) {
	LED_HEIGH_PORT.DIRSET = LED_HEIGH_MASK;
	LED_LOW_PORT.DIRSET = LED_LOW_MASK;
}

static inline
void CARDHEIGHSetOn(uint8_t Mask) {
	LED_HEIGH_PORT.OUTSET = Mask;
}

static inline
void CARDLOWSetOn(uint8_t Mask) {
	LED_LOW_PORT.OUTSET = Mask;
}

static inline
void CARDHEIGHSetOff(uint8_t Mask) {
	LED_HEIGH_PORT.OUTCLR = Mask;
}

static inline
void CARDLOWSetOff(uint8_t Mask) {
	LED_LOW_PORT.OUTCLR = Mask;
}

#endif /* LED_H */
