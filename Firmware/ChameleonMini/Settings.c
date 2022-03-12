/*
 * Settings.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 *
 */

#include "Settings.h"
#include <avr/eeprom.h>
#include "Configuration.h"
#include <string.h>
#include "Memory/Memory.h"
#include "Terminal/CommandLine.h"
#include "System.h"

SettingsType GlobalSettings;

SettingsType EEMEM StoredSettings = {
    .ActiveSettingIdx = SETTING_TO_INDEX(DEFAULT_SETTING),
    .ActiveSettingPtr = &GlobalSettings.Settings[SETTING_TO_INDEX(DEFAULT_SETTING)],
    .UidMode = 0,

    .Settings = { [0 ...(SETTINGS_COUNT - 1)] =  {
            .Configuration = DEFAULT_CONFIGURATION,
            .ButtonAction = DEFAULT_BUTTON_ACTION,
            .ButtonLongAction = DEFAULT_BUTTON_LONG_ACTION,
            .PendingTaskTimeout = SETTINGS_TIMEOUT,
            }
        }
};

void SettingsLoad(void) {
    eeprom_read_block(&GlobalSettings, &StoredSettings, sizeof(SettingsType));
}

void SettingsSave(void) {
    eeprom_write_block(&GlobalSettings, &StoredSettings, sizeof(SettingsType));
}

void SettingsCycle(void) {
    char i = SETTINGS_COUNT;
    uint8_t SettingIdx = GlobalSettings.ActiveSettingIdx;

    while (i-- > SETTINGS_FIRST) {
        SettingIdx = (SettingIdx + 1) % SETTINGS_COUNT;

        if (GlobalSettings.Settings[SettingIdx].Configuration != CONFIG_NONE) {
            if (SettingsSetActiveById(INDEX_TO_SETTING(SettingIdx))) {
                SettingsSave();
            }
            break;
        }
    }
}

bool SettingsSetActiveById(uint8_t Setting) {
    if ( (Setting >= SETTINGS_FIRST) && (Setting <= SETTINGS_LAST) ) {
        GlobalSettings.ActiveSettingIdx = SETTING_TO_INDEX(Setting);
        GlobalSettings.ActiveSettingPtr = &GlobalSettings.Settings[GlobalSettings.ActiveSettingIdx];

        /* Settings have changed. Progress changes through system */
        ConfigurationInit();
        return true;
    } else {
        return false;
    }
}

uint8_t SettingsGetActiveById(void) {
    return INDEX_TO_SETTING(GlobalSettings.ActiveSettingIdx);
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

    if ((Setting[1] == '\0') && (SettingNr <= SETTINGS_LAST)) {
        return SettingsSetActiveById(SettingNr);
    } else {
        return false;
    }
}

void SettingsSetUidMode(bool isActive) {
    GlobalSettings.UidMode = isActive;
    SettingsSave();
}
