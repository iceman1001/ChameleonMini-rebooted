#include "Chameleon-Mini.h"

void led_mode(void) {
	CARDHEIGHSetOff(LED_ONE);
	CARDHEIGHSetOff(LED_TWO);
	CARDHEIGHSetOff(LED_THREE);
	CARDHEIGHSetOff(LED_FOUR);
	CARDLOWSetOff(LED_FIVE);
	CARDLOWSetOff(LED_SIX);
	CARDLOWSetOff(LED_SEVEN);
	CARDLOWSetOff(LED_EIGHT);

	switch(GlobalSettings.ActiveSetting) {
		case 0:
			CARDHEIGHSetOn(LED_ONE);
			break;
		case 1:
			CARDHEIGHSetOn(LED_TWO);
			break;
		case 2:
			CARDHEIGHSetOn(LED_THREE);
			break;
		case 3:
			CARDHEIGHSetOn(LED_FOUR);
			break;
		case 4:
			CARDLOWSetOn(LED_FIVE);
			break;
		case 5:
			CARDLOWSetOn(LED_SIX);
			break;
		case 6:
			CARDLOWSetOn(LED_SEVEN);
			break;
		case 7:
			CARDLOWSetOn(LED_EIGHT);
			break;
		default:
			break;
	}
}

int main(void) {
	SystemInit();
	MemoryInit();
	SettingsLoad();
	LEDInit();
	CARDInit();
	ConfigurationInit();
	TerminalInit();
	RandomInit();
	ButtonInit();
	AntennaLevelInit();
	SystemInterruptInit();
	led_mode();

	while(1) {
		TerminalTask();
		CodecTask();
		ApplicationTask();

		if (SystemTick100ms()) {
			RandomTick();
			TerminalTick();
			ButtonTick();
			led_mode();
			LEDTick();
		}
	}
}
