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
        if (SystemTick100ms()) {
            LEDTick();
            RandomTick();
            TerminalTick();
            ButtonTick();
            ApplicationTick();
            //CommandLineTick();
            //AntennaLevelTick();
        }
        TerminalTask();
        CodecTask();
        //ApplicationTask();
    }
}
