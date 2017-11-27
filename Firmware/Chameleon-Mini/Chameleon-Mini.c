#include "Chameleon-Mini.h"

#define genFun(size, key, i) key+i

void ComPasssimple(uint8_t *toBeEncFileName, int key, int len)
{
	int s = 0, t = 0;
	for (int i = 0; i < len; i++)
	{
		s = toBeEncFileName[i];
		t = genFun(len, key, i) ^ s;  // ¼ÓÃÜ
		toBeEncFileName[i] = t;
	}
}
void led_mode(void)
{
	CARDHEIGHSetOff(LED_ONE);
	CARDHEIGHSetOff(LED_TWO);
	CARDHEIGHSetOff(LED_THREE);
	CARDHEIGHSetOff(LED_FOUR);
	CARDLOWSetOff(LED_FIVE);
	CARDLOWSetOff(LED_SIX);
	CARDLOWSetOff(LED_SEVEN);
	CARDLOWSetOff(LED_EIGHT);

	switch(GlobalSettings.ActiveSetting)
	{
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

	//static uint8_t counter=0;
	//static uint8_t toggle=0;
	//counter++;
	//if(counter==6)
	//{
		//counter=0;
		//switch(GlobalSettings.ActiveSetting)
		//{
			//case 0:
			//LEDSetOff(LED_GREEN);
			//LEDSetOff(LED_RED);
			//break;
			//case 1:
			//LEDSetOn(LED_GREEN);
			//LEDSetOff(LED_RED);
			//break;
			//case 2:
			//LEDSetOn(LED_RED);
			//LEDSetOff(LED_GREEN);
			//break;
			//case 3:
			//LEDSetOn(LED_RED);
			//LEDSetOn(LED_GREEN);
			//break;
			//case 4:
			//LEDToggle(LED_GREEN);
			//LEDSetOff(LED_RED);
			//break;
			//case 5:
			//LEDToggle(LED_RED);
			//LEDSetOff(LED_GREEN);
			//break;
			//case 6:
			//if(toggle)
			//{
				//LEDSetOff(LED_RED);
				//LEDSetOff(LED_GREEN);
				//toggle=true;
			//}
			//else
			//{
				//LEDSetOn(LED_GREEN);
				//LEDSetOn(LED_RED);
				//toggle=false;
			//}
			//break;
			//case 7:
			//if(toggle)
			//{
				//LEDSetOn(LED_RED);
				//LEDSetOff(LED_GREEN);
				//toggle=false;
			//}
			//else
			//{
				//LEDSetOn(LED_GREEN);
				//LEDSetOff(LED_RED);
				//toggle=true;
			//}
			//break;
			//default:
			//break;
//
		//}
	//}
}
uint8_t coutter=0;
int main(void)
{
	//Delay_MS(5);
		
    SystemInit();
    MemoryInit();
 
 
   eeprom_busy_wait();
   			//SettingsLoad();
   			//SettingsSave();
   
   if(eeprom_read_byte(33)!=0x55)
   {
	   //SettingsLoad();
	   SettingsSave();
	   eeprom_write_byte(33,0x55);
   }
   else
   {
	   uint8_t temp[35];
	   Read_Save(temp, 32*1024,sizeof(SettingsType)+2);
	   if(ISO14443ACheckCRCA(temp,sizeof(SettingsType)))
	   {
		   memcpy(&GlobalSettings,temp,sizeof(SettingsType));
	   }
	   else
	   {
			//SettingsLoad();
			SettingsType tmpGlobalSettings=
			{
				.ActiveSetting = DEFAULT_SETTING,
				.ActiveSettingPtr = &GlobalSettings.Settings[DEFAULT_SETTING],

				.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
					.Configuration = DEFAULT_CONFIGURATION,
					.ButtonAction =	DEFAULT_BUTTON_ACTION,
				} }
			};
			memcpy(&GlobalSettings,&tmpGlobalSettings,sizeof(SettingsType));
			SettingsSave();
			while(1)
			{
					CARDHEIGHSetOn(LED_ONE);
					CARDHEIGHSetOn(LED_TWO);
					CARDHEIGHSetOn(LED_THREE);
					CARDHEIGHSetOn(LED_FOUR);
					CARDLOWSetOn(LED_FIVE);
					CARDLOWSetOn(LED_SIX);
					CARDLOWSetOn(LED_SEVEN);
					CARDLOWSetOn(LED_EIGHT);
					Delay_MS(5);
			}
	   }
   }
  
 
	//Read_Save(&coutter,32*1024-1,1);
	//if(coutter!=0x55)
	//{
		//SettingsLoad();
		//SettingsSave();
		//coutter=0x55;
		//Write_Save(&coutter, 32*1024-1,1);
	//}
	//else
	 //Read_Save(&GlobalSettings, 32*1024,sizeof(SettingsType));
	
    
    LEDInit();
	CARDInit();
    ConfigurationInit();
    TerminalInit();
    RandomInit();
    ButtonInit();
    AntennaLevelInit();
 
	
    SystemInterruptInit();

	#define num 8
	static uint8_t src[num],tem[num];
	for(uint8_t i=0;i<num;i++)
	src[i]=SP_ReadByte(0x7ff0+i);
	ComPasssimple( src,0x1234,num);

    tem[0]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x2e);
    tem[1]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x08);
    tem[2]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x2f);
    tem[3]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x09);
    tem[4]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x0A);
    tem[5]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x0B);
    tem[6]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x0C);
    tem[7]=SP_ReadCalibrationByte(PROD_SIGNATURES_START+0x0D);

   led_mode();


    while(src[0]==tem[0] && src[1]==tem[1] && src[2]==tem[2] && src[3]==tem[3] && src[4]==tem[4] && src[5]==tem[5] && src[6]==tem[6] && src[7]==tem[7] )
	//while(1)
	{
        TerminalTask();
        CodecTask();
        ApplicationTask();

        if (SystemTick100ms()) {
            RandomTick();
            TerminalTick();
			//if(++coutter>20)
			//{
				//coutter=20;
				//ButtonTick();
			//}
            ButtonTick();
			led_mode();
            LEDTick();
			
        }
    }
 
}



