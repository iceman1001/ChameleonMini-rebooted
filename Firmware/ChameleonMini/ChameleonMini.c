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
#ifdef CONFIG_LEGIC_PRIME_SUPPORT
        ApplicationTask();
#endif
    }
}
