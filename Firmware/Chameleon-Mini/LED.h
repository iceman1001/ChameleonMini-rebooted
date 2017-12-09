/*
 * LED.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef LED_H
#define LED_H

#include <avr/io.h>
#include "Common.h"

#define LED_PORT PORTA
#define LED_GREEN	PIN5_bm
#define LED_RED		PIN4_bm
#define LED_MASK	(LED_GREEN | LED_RED)


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
			
typedef enum LEDFunctionEnum {
    LED_NO_FUNC = 0,		/* Don't light up the LED */
    LED_POWERED,			/* Light up the LED whenever the Chameleon is powered */

    LED_TERMINAL_CONN,		/* A Terminal/USB connection has been established */
    LED_TERMINAL_RXTX,		/* There is traffic on the terminal */

    LED_SETTING_CHANGE,		/* Outputs a blink code that shows the current setting */

    LED_MEMORY_STORED, 		/* Blink once when memory has been stored to flash */
    LED_MEMORY_CHANGED, 	/* Switch LED on when card memory has changed compared to flash */

    LED_FIELD_DETECTED, 	/* Shows LED while a reader field is being detected or turned on by the chameleon itself */

    LED_CODEC_RX,			/* Blink LED when receiving codec data */
    LED_CODEC_TX,			/* Blink LED when transmitting codec data */

    LED_LOG_MEM_FULL,		/* Light up if log memory is full. */

    //TODO: LED_APP_SELECTED,		/* Show LED while the correct UID has been selected and the application is active */
    /* Has to be last element */
    LED_FUNC_COUNT
} LEDHookEnum;

typedef enum LEDActionEnum {
    LED_NO_ACTION = 0x00,
    LED_OFF = 0x10,
    LED_ON = 0x11,
    LED_TOGGLE = 0x12,
    LED_PULSE = 0x13,
    LED_BLINK = 0x20,
    LED_BLINK_1X = 0x20,
    LED_BLINK_2X, /* Have to be sequentially ordered */
    LED_BLINK_3X,
    LED_BLINK_4X,
    LED_BLINK_5X,
    LED_BLINK_6X,
    LED_BLINK_7X,
    LED_BLINK_8X,
} LEDActionEnum;


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



extern uint8_t LEDPulseMask;

void LEDInit(void);

static inline
void LEDSetOn(uint8_t Mask) {
    LED_PORT.OUTSET = Mask;
}

static inline
void LEDSetOff(uint8_t Mask) {
    LED_PORT.OUTCLR = Mask;
}

static inline
void LEDToggle(uint8_t Mask) {
    LED_PORT.OUTTGL = Mask;
}


static inline
void LEDPulse(uint8_t Mask) {
	LEDPulseMask = Mask;
	LED_PORT.OUTSET = Mask;
}


void LEDTick(void);

#endif /* LED_H */
