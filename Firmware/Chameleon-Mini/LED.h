/*
 * LED.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef LED_H
#define LED_H

#include "Common.h"
#include "Settings.h"
#include <avr/io.h>

#define LED_HIGH_PORT  PORTA
#define LED_ONE         PIN5_bm
#define LED_TWO         PIN4_bm
#define LED_THREE       PIN3_bm
#define LED_FOUR        PIN2_bm
#define LED_HIGH_MASK   (LED_ONE | LED_TWO | LED_THREE| LED_FOUR)

#define LED_LOW_PORT    PORTE
#define LED_FIVE        PIN3_bm
#define LED_SIX         PIN2_bm
#define LED_SEVEN       PIN1_bm
#define LED_EIGHT       PIN0_bm
#define LED_LOW_MASK    (LED_FIVE | LED_SIX | LED_SEVEN| LED_EIGHT)

void LEDMode(void);
void LEDInit(void);
void LEDTick(void);
void CARDInit(void);
void LEDHighSetOn(uint8_t Mask);
void LEDLowSetOn(uint8_t Mask);
void LEDHighSetOff(uint8_t Mask);
void LEDLowSetOff(uint8_t Mask);

#endif /* LED_H */
