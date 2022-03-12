/*
 * Settings.h
 *
 *  Created on: 21.12.2013
 *      Author: skuser
 */
/** @file */
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Button.h"
#include "Configuration.h"

#define SETTINGS_COUNT                  8
#define SETTINGS_FIRST                  0
#define SETTINGS_LAST                   (SETTINGS_FIRST + SETTINGS_COUNT - 1)
#ifdef DEFAULT_PENDING_TASK_TIMEOUT
#define SETTINGS_TIMEOUT                DEFAULT_PENDING_TASK_TIMEOUT
#else
#define SETTINGS_TIMEOUT                50 // *100ms
#endif

#define SETTING_TO_INDEX(S) (S - SETTINGS_FIRST)
#define INDEX_TO_SETTING(I) (I + SETTINGS_FIRST)

/** Defines one setting.
 *
 * \note Some properties may change globally if this is defined in the Makefile.
 */
typedef struct {
    ButtonActionEnum ButtonAction; /// Button action for this setting.
    ButtonActionEnum ButtonLongAction; /// Long button action for this setting.
    ConfigurationEnum Configuration; /// Active configuration for this setting.
    uint16_t PendingTaskTimeout; /// Timeout for timeout commands for this setting, in multiples of 100 ms.
} SettingsEntryType;

typedef struct {
    uint8_t ActiveSettingIdx;
    SettingsEntryType* ActiveSettingPtr;
    SettingsEntryType Settings[SETTINGS_COUNT];
    bool UidMode;
} SettingsType;

extern SettingsType GlobalSettings;

void SettingsLoad(void);
void SettingsSave(void);
void ActiveSettingNumberSave(void);

void SettingsCycle(void);
bool SettingsSetActiveById(uint8_t Setting);
uint8_t SettingsGetActiveById(void);
void SettingsGetActiveByName(char* SettingOut, uint16_t BufferSize);
bool SettingsSetActiveByName(const char* Setting);

void SettingsSetUidMode(bool isActive);

#endif /* SETTINGS_H_ */
