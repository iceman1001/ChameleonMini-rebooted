#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "LED.h"
#include "Settings.h"
#include "Common.h"

static const LedPining_t LEDPiningTable[] PROGMEM = {
    [LED_ONE] =   { .ledPort = &LED_HIGH_PORT, .ledPin = PIN5_bm },
    [LED_TWO] =   { .ledPort = &LED_HIGH_PORT, .ledPin = PIN4_bm },
    [LED_THREE] = { .ledPort = &LED_HIGH_PORT, .ledPin = PIN3_bm },
    [LED_FOUR] =  { .ledPort = &LED_HIGH_PORT, .ledPin = PIN2_bm },
    [LED_FIVE] =  { .ledPort = &LED_LOW_PORT,  .ledPin = PIN3_bm },
    [LED_SIX] =   { .ledPort = &LED_LOW_PORT,  .ledPin = PIN2_bm },
    [LED_SEVEN] = { .ledPort = &LED_LOW_PORT,  .ledPin = PIN1_bm },
    [LED_EIGHT] = { .ledPort = &LED_LOW_PORT,  .ledPin = PIN0_bm }
};

INLINE void LEDAllOff(void) {
    LED_HIGH_PORT.OUTCLR = LED_HIGH_MASK;
    LED_LOW_PORT.OUTCLR = LED_LOW_MASK;
}

INLINE void LEDMode(void) {
    LEDAllOff();
    LEDSetOn(GlobalSettings.ActiveSettingIdx);
}

void LEDTick(void) {
    LED_HIGH_PORT.OUTCLR = LED_LOW_PULSE_MASK;
    LED_LOW_PORT.OUTCLR = LED_HIGH_PULSE_MASK;
    LEDMode();
}

void LEDInit(void) {
    LED_LOW_PORT.DIRSET = LED_LOW_MASK;
    LED_HIGH_PORT.DIRSET = LED_HIGH_MASK;
}

INLINE void LEDSet(Led LedId, bool on) {
    PORT_t * ledPort = ( (PORT_t *)pgm_read_ptr( &(LEDPiningTable[LedId].ledPort) ) );
    uint8_t ledPin = ( (uint8_t)pgm_read_byte( &(LEDPiningTable[LedId].ledPin) ) );
    (on) ? (ledPort->OUTSET = ledPin) : (ledPort->OUTCLR = ledPin);
}

void LEDSetOn(Led LedId) {
    LEDSet(LedId, true);
}
void LEDSetOff(Led LedId) {
    LEDSet(LedId, false);
}
