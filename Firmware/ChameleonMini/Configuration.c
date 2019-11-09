/*
 * Configuration.c
 *
 *  Created on: 15.02.2013
 *      Author: skuser
 */

#include <avr/pgmspace.h>
#include "Configuration.h"
#include "Settings.h"
#include "Map.h"

/* Map IDs to text */
static const MapEntryType ConfigurationMap[] PROGMEM = {
    { .Id = CONFIG_NONE,                     .Text = "CLOSED" },
#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    { .Id = CONFIG_MF_ULTRALIGHT,            .Text = "MF_ULTRALIGHT" },
    { .Id = CONFIG_MF_ULTRALIGHT_EV1_80B,    .Text = "MF_ULTRALIGHT_EV1_80B" },
    { .Id = CONFIG_MF_ULTRALIGHT_EV1_164B,   .Text = "MF_ULTRALIGHT_EV1_164B" },
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_1K,            .Text = "MF_CLASSIC_1K" },
#endif
#ifdef CONFIG_MF_CLASSIC_1K_7B_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_1K_7B,         .Text = "MF_CLASSIC_1K_7B" },
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_4K,            .Text = "MF_CLASSIC_4K" },
#endif
#ifdef CONFIG_MF_CLASSIC_4K_7B_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_4K_7B,         .Text = "MF_CLASSIC_4K_7B" },
#endif
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_MINI,          .Text = "MF_CLASSIC_MINI" },
#endif
#ifdef CONFIG_MF_DETECTION_SUPPORT
    { .Id = CONFIG_MF_DETECTION,             .Text = "MF_DETECTION" },
#endif
#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
    { .Id = CONFIG_MF_CLASSIC_BRUTE,         .Text = "MF_CLASSIC_BRUTE" },
#endif
};

/* Include all Codecs and Applications */
#include "Codec/Codec.h"
#include "Application/Application.h"

static void CodecInitDummy(void) { }
static void CodecTaskDummy(void) { }
static void ApplicationInitDummy(void) {}
static void ApplicationResetDummy(void) {}
static void ApplicationTaskDummy(void) {}
static void ApplicationTickDummy(void) {}
static void ApplicationButtonFuncDummy(void) {}
static uint16_t ApplicationProcessDummy(uint8_t* ByteBuffer, uint16_t ByteCount) { return CONFIGURATION_DUMMY_UID_PART; }
static void ApplicationGetUidDummy(ConfigurationUidType Uid) { memset(Uid, CONFIGURATION_DUMMY_UID_PART, CONFIGURATION_DUMMY_UID_SIZE); }
static void ApplicationSetUidDummy(ConfigurationUidType Uid) { }
static void ApplicationGetAtqaDummy(uint16_t * Atqa) { *Atqa = CONFIGURATION_DUMMY_ATQA; }
static void ApplicationSetAtqaDummy(uint16_t Atqa) { }
static void ApplicationGetSakDummy(uint8_t * Sak) { *Sak = CONFIGURATION_DUMMY_SAK; }
static void ApplicationSetSakDummy(uint8_t Sak) { }

static const PROGMEM ConfigurationType ConfigurationTable[] = {
[CONFIG_NONE] = {
    .CodecInitFunc = CodecInitDummy,
    .CodecTaskFunc = CodecTaskDummy,
    .ApplicationInitFunc = ApplicationInitDummy,
    .ApplicationResetFunc = ApplicationResetDummy,
    .ApplicationTaskFunc = ApplicationTaskDummy,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = ApplicationProcessDummy,
    .ApplicationGetUidFunc = ApplicationGetUidDummy,
    .ApplicationSetUidFunc = ApplicationSetUidDummy,
    .ApplicationGetSakFunc = ApplicationGetSakDummy,
    .ApplicationSetSakFunc = ApplicationSetSakDummy,
    .ApplicationGetAtqaFunc = ApplicationGetAtqaDummy,
    .ApplicationSetAtqaFunc = ApplicationSetAtqaDummy,
    .UidSize = CONFIGURATION_DUMMY_UID_SIZE,
    .MemorySize = CONFIGURATION_DUMMY_MEMSIZE,
    .ReadOnly = true
},
#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
[CONFIG_MF_ULTRALIGHT] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareUltralightAppInit,
    .ApplicationResetFunc = MifareUltralightAppReset,
    .ApplicationTaskFunc = MifareUltralightAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareUltralightAppProcess,
    .ApplicationGetUidFunc = MifareUltralightGetUid,
    .ApplicationSetUidFunc = MifareUltralightSetUid,
    .ApplicationGetSakFunc = MifareUltralightGetSak,
    .ApplicationSetSakFunc = MifareUltralightSetSak,
    .ApplicationGetAtqaFunc = MifareUltralightGetAtqa,
    .ApplicationSetAtqaFunc = MifareUltralightSetAtqa,
    .UidSize = MIFARE_ULTRALIGHT_UID_SIZE,
    .MemorySize = MIFARE_ULTRALIGHT_MEM_SIZE,
    .ReadOnly = false
},
[CONFIG_MF_ULTRALIGHT_EV1_80B] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareUltralightEV11AppInit,
    .ApplicationResetFunc = MifareUltralightAppReset,
    .ApplicationTaskFunc = MifareUltralightAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareUltralightAppProcess,
    .ApplicationGetUidFunc = MifareUltralightGetUid,
    .ApplicationSetUidFunc = MifareUltralightSetUid,
    .ApplicationGetSakFunc = MifareUltralightGetSak,
    .ApplicationSetSakFunc = MifareUltralightSetSak,
    .ApplicationGetAtqaFunc = MifareUltralightGetAtqa,
    .ApplicationSetAtqaFunc = MifareUltralightSetAtqa,
    .UidSize = MIFARE_ULTRALIGHT_UID_SIZE,
    .MemorySize = MIFARE_ULTRALIGHT_EV11_MEM_SIZE,
    .ReadOnly = false
},
[CONFIG_MF_ULTRALIGHT_EV1_164B] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareUltralightEV12AppInit,
    .ApplicationResetFunc = MifareUltralightAppReset,
    .ApplicationTaskFunc = MifareUltralightAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareUltralightAppProcess,
    .ApplicationGetUidFunc = MifareUltralightGetUid,
    .ApplicationSetUidFunc = MifareUltralightSetUid,
    .ApplicationGetSakFunc = MifareUltralightGetSak,
    .ApplicationSetSakFunc = MifareUltralightSetSak,
    .ApplicationGetAtqaFunc = MifareUltralightGetAtqa,
    .ApplicationSetAtqaFunc = MifareUltralightSetAtqa,
    .UidSize = MIFARE_ULTRALIGHT_UID_SIZE,
    .MemorySize = MIFARE_ULTRALIGHT_EV12_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
[CONFIG_MF_CLASSIC_1K] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppInit1K,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_SIZE,
    .MemorySize = MFCLASSIC_1K_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_1K_7B_SUPPORT
[CONFIG_MF_CLASSIC_1K_7B] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppInit1K,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_7B_SIZE,
    .MemorySize = MFCLASSIC_1K_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
[CONFIG_MF_CLASSIC_4K] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppInit4K,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_SIZE,
    .MemorySize = MFCLASSIC_4K_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_4K_7B_SUPPORT
[CONFIG_MF_CLASSIC_4K_7B] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppInit4K,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_7B_SIZE,
    .MemorySize = MFCLASSIC_4K_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
[CONFIG_MF_CLASSIC_MINI] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppInitMini,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_SIZE,
    .MemorySize = MFCLASSIC_MINI_MEM_SIZE,
    .ReadOnly = false
},
#endif
#ifdef CONFIG_MF_DETECTION_SUPPORT
[CONFIG_MF_DETECTION] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppDetectionInit,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = ApplicationButtonFuncDummy,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_SIZE,
    .MemorySize = DETECTION_MEM_APP_SIZE,
    .ReadOnly = true
},
#endif
#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
[CONFIG_MF_CLASSIC_BRUTE] = {
    .CodecInitFunc = ISO14443ACodecInit,
    .CodecTaskFunc = ISO14443ACodecTask,
    .ApplicationInitFunc = MifareClassicAppBruteInit,
    .ApplicationResetFunc = MifareClassicAppReset,
    .ApplicationTaskFunc = MifareClassicAppTask,
    .ApplicationTickFunc = ApplicationTickDummy,
    .ApplicationButtonFunc = MifareClassicAppBruteToggle,
    .ApplicationProcessFunc = MifareClassicAppProcess,
    .ApplicationGetUidFunc = MifareClassicGetUid,
    .ApplicationSetUidFunc = MifareClassicSetUid,
    .ApplicationGetSakFunc = MifareClassicGetSak,
    .ApplicationSetSakFunc = MifareClassicSetSak,
    .ApplicationGetAtqaFunc = MifareClassicGetAtqa,
    .ApplicationSetAtqaFunc = MifareClassicSetAtqa,
    .UidSize = MFCLASSIC_UID_SIZE,
    .MemorySize = BRUTE_MEM_SIZE,
    .ReadOnly = false
},
#endif
};

ConfigurationType ActiveConfiguration;

void ConfigurationInit(void)
{
    ConfigurationSetById(GlobalSettings.ActiveSettingPtr->Configuration);
}

void ConfigurationSetById( ConfigurationEnum Configuration )
{
    GlobalSettings.ActiveSettingPtr->Configuration = Configuration;

    /* Copy struct from PROGMEM to RAM */
    memcpy_P(&ActiveConfiguration, &ConfigurationTable[Configuration], sizeof(ConfigurationType));

    CodecInit();
    ApplicationInit();
}

void ConfigurationGetByName(char* Configuration, uint16_t BufferSize)
{
    MapIdToText(ConfigurationMap, ARRAY_COUNT(ConfigurationMap), GlobalSettings.ActiveSettingPtr->Configuration, Configuration, BufferSize);
}

bool ConfigurationSetByName(const char* Configuration)
{
    MapIdType Id;
    if (MapTextToId(ConfigurationMap, ARRAY_COUNT(ConfigurationMap), Configuration, &Id)) {
        ConfigurationSetById(Id);
        return true;
    } else {
        return false;
    }
}

void ConfigurationGetList(char* List, uint16_t BufferSize)
{
    MapToString(ConfigurationMap, ARRAY_COUNT(ConfigurationMap), List, BufferSize);
}

uint32_t ConfigurationTableGetMemorySizeForId(ConfigurationEnum Configuration) {
    /* Possible other implementation
    ConfigurationType ConfForSetting;
    memcpy_P( &ConfForSetting,
              &(ConfigurationTable[GlobalSettings.Settings[SettingNumber].Configuration]),
              sizeof(ConfigurationType) );
    return ConfForSetting.MemorySize;
    */
    return ( (uint32_t)pgm_read_dword( &(ConfigurationTable[Configuration].MemorySize) ) );
}

