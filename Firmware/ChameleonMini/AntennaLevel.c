#include "AntennaLevel.h"
#include "Application/Application.h"

void AntennaLevelInit(void) {
    ADCA.CTRLA = ADC_ENABLE_bm;
    ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;
    ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_BANDGAP_bm;
    ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc;
    ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;
}

uint16_t AntennaLevelGet(void) {
    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) );

    ADCA.CH0.INTFLAGS = ADC_CH_CHIF_bm;

    int16_t Result = ADCA.CH0RES - ANTENNA_LEVEL_OFFSET;
    if (Result < 0) Result = 0;

    return (uint16_t) (((uint32_t) Result * ANTENNA_LEVEL_NUMERATOR) / ANTENNA_LEVEL_DENOMINATOR);
}

void AntennaLevelTick(void) {
    uint16_t rssi = AntennaLevelGet();

    // UidSize != 0 implies that we are emulating right now
    if( (rssi < ANTENNA_FIELD_MIN_RSSI) && (ActiveConfiguration.UidSize != 0) ) {
        ApplicationReset(); // reset the application just like a real card gets reset when there is no field
    }
}
