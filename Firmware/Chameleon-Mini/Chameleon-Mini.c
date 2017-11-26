#include "Chameleon-Mini.h"

 

int main(void)
{
    SystemInit();
    SettingsLoad();
    LEDInit();
    MemoryInit();
    ConfigurationInit();
    TerminalInit();
    RandomInit();
    ButtonInit();
    AntennaLevelInit();
	uart_init(); 
 
	
    SystemInterruptInit();

 

 
	while(1)
	{
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



