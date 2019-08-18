#include "ChameleonMini.h"

int main(void) {
	SystemInit();
	MemoryInit();
	SettingsLoad();
	LEDInit();
	ConfigurationInit();
	TerminalInit();
	RandomInit();
	ButtonInit();
	AntennaLevelInit();
	SystemInterruptInit();

	while(1) {
		TerminalTask();
		CodecTask();
		ApplicationTask();

		if (SystemTick100ms()) {
			RandomTick();
			TerminalTick();
			ButtonTick();
			LEDTick();
		}
	}
}
