#include "LED.h"

uint8_t LEDLowPulseMask = 0, LEDHighPulseMask = 0;

inline
void LEDMode(void) {
	LEDHighSetOff(LED_ONE);
	LEDHighSetOff(LED_TWO);
	LEDHighSetOff(LED_THREE);
	LEDHighSetOff(LED_FOUR);
	LEDLowSetOff(LED_FIVE);
	LEDLowSetOff(LED_SIX);
	LEDLowSetOff(LED_SEVEN);
	LEDLowSetOff(LED_EIGHT);

	switch(GlobalSettings.ActiveSetting) {
		case 0: LEDHighSetOn(LED_ONE); break;
		case 1: LEDHighSetOn(LED_TWO); break;
		case 2: LEDHighSetOn(LED_THREE); break;
		case 3: LEDHighSetOn(LED_FOUR); break;
		case 4: LEDLowSetOn(LED_FIVE); break;
		case 5:	LEDLowSetOn(LED_SIX); break;
		case 6:	LEDLowSetOn(LED_SEVEN); break;
		case 7:	LEDLowSetOn(LED_EIGHT); break;
		default: break;
	}
}

void LEDInit(void) {
	LED_LOW_PORT.DIRSET = LED_LOW_MASK;
	LED_HIGH_PORT.DIRSET = LED_HIGH_MASK;
}

inline
void LEDTick(void) {
	LED_HIGH_PORT.OUTCLR = LEDLowPulseMask;
	LEDLowPulseMask = 0;
	LED_LOW_PORT.OUTCLR = LEDHighPulseMask;
	LEDHighPulseMask = 0;
	LEDMode();
}

inline
void LEDHighSetOn(uint8_t Mask) {
	LED_HIGH_PORT.OUTSET = Mask;
}

inline
void LEDLowSetOn(uint8_t Mask) {
	LED_LOW_PORT.OUTSET = Mask;
}

inline
void LEDHighSetOff(uint8_t Mask) {
	LED_HIGH_PORT.OUTCLR = Mask;
}

inline
void LEDLowSetOff(uint8_t Mask) {
	LED_LOW_PORT.OUTCLR = Mask;
}
