#include "Settings.h"
#include <avr/eeprom.h>
#include "Configuration.h"
#include <string.h>
#include "LED.h"
#include "Memory.h"
#include "Terminal/CommandLine.h"
#include "System.h"

#define SETTING_TO_INDEX(S) (S - SETTINGS_FIRST)
#define INDEX_TO_SETTING(I) (I + SETTINGS_FIRST)														

SettingsType GlobalSettings = {
	.ActiveSetting = DEFAULT_SETTING,
	.ActiveSettingPtr = &GlobalSettings.Settings[DEFAULT_SETTING],
	.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
				.Configuration = DEFAULT_CONFIGURATION,
				.ButtonAction = DEFAULT_BUTTON_ACTION,
				.ButtonLongAction = DEFAULT_BUTTON_LONG_ACTION,
				} 
			}
};
SettingsType EEMEM StoredSettings = {
	.ActiveSetting = DEFAULT_SETTING,
	.ActiveSettingPtr = &GlobalSettings.Settings[DEFAULT_SETTING],

	.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
			.Configuration = DEFAULT_CONFIGURATION,
			.ButtonAction = DEFAULT_BUTTON_ACTION,
			.ButtonLongAction = DEFAULT_BUTTON_LONG_ACTION,
			}
		}
};

void SettingsLoad(void) {
	eeprom_read_block(&GlobalSettings, &StoredSettings, sizeof(SettingsType));
}

void SettingsSave(void) {
#if ENABLE_EEPROM_SETTINGS
	eeprom_write_block(&GlobalSettings, &StoredSettings, sizeof(SettingsType));
#endif
}

void SettingsCycle(void) {
	char i = SETTINGS_COUNT;
	uint8_t Setting = GlobalSettings.ActiveSetting;

	while (i-- > 0) {
		Setting = (Setting + 1) % SETTINGS_COUNT;

		if (GlobalSettings.Settings[Setting].Configuration != CONFIG_NONE) {
			if (SettingsSetActiveById(Setting)) {
				SettingsSave();
			}
			break;
		}
	}
}

bool SettingsSetActiveById(uint8_t Setting) {
	if ( (Setting >= SETTINGS_FIRST) && (Setting <= SETTINGS_LAST) ) {
		GlobalSettings.ActiveSetting = Setting;
		GlobalSettings.ActiveSettingPtr = &GlobalSettings.Settings[GlobalSettings.ActiveSetting];

		/* Settings have changed. Progress changes through system */
		ConfigurationInit();
		return true;
	} else {
		return false;
	}
}

uint8_t SettingsGetActiveById(void) {
	return GlobalSettings.ActiveSetting;
}

void SettingsGetActiveByName(char* SettingOut, uint16_t BufferSize) {
	SettingOut[0] = 'N';
	SettingOut[1] = 'O';
	SettingOut[2] = '.';	
	SettingOut[3] = SettingsGetActiveById() + '0';
	SettingOut[4] = '\0';
}

bool SettingsSetActiveByName(const char* Setting) {
	uint8_t SettingNr = Setting[0] - '0';

	if ((Setting[1] == '\0') && (SettingNr < SETTINGS_COUNT)) {
		return SettingsSetActiveById(SettingNr);
	} else {
		return false;
	}
}
