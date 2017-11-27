

#ifndef LED_H
#define LED_H

#include <avr/io.h>

#define LED_PORT PORTA
#define LED_GREEN	PIN5_bm
#define LED_RED		PIN4_bm
#define LED_MASK	(LED_GREEN | LED_RED)
//#define LED_PORT PORTE
//#define LED_GREEN	PIN2_bm
//#define LED_RED		PIN3_bm
//#define LED_MASK	(LED_GREEN | LED_RED)

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

static inline
void LEDInit(void) {
    LED_PORT.DIRSET = LED_MASK;
}

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

static inline
void LEDTick(void) {
	LED_PORT.OUTCLR = LEDPulseMask;
	LEDPulseMask = 0;
}

#endif /* LED_H */
