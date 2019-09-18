/*
 * MifareClassic.h
 *
 * 2019, @shinhub
 *
 * Parts are Created on: 20.03.2013, Author: skuser
 *
 */

#ifndef MIFARECLASSIC_H_
#define MIFARECLASSIC_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define MIFARE_CLASSIC_UID_SIZE       ISO14443A_UID_SIZE_SINGLE
#define MIFARE_CLASSIC_1K_MEM_SIZE    1024
#define MIFARE_CLASSIC_4K_MEM_SIZE    4096
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
#define MIFARE_CLASSIC_MINI_MEM_SIZE  320
#endif

#define MFCLASSIC_1K_ATQA_VALUE       0x0004
#define MFCLASSIC_1K_7B_ATQA_VALUE    0x0044
#define MFCLASSIC_4K_ATQA_VALUE       0x0002
#define MFCLASSIC_4K_7B_ATQA_VALUE    0x0042
#define MFCLASSIC_1K_SAK_CL1_VALUE    0x08
#define MFCLASSIC_4K_SAK_CL1_VALUE    0x18
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
#define MFCLASSIC_MINI_ATQA_VALUE     MFCLASSIC_1K_ATQA_VALUE
#define MFCLASSIC_MINI_SAK_VALUE      0x09
#endif

void MifareClassicAppInit1K(void);
void MifareClassicAppInit4K(void);
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
void MifareClassicAppInitMini(void);
#endif
void MifareClassicAppReset(void);
void MifareClassicAppTask(void);

uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareClassicGetUid(ConfigurationUidType Uid);
void MifareClassicSetUid(ConfigurationUidType Uid);

void MifareClassicGetAtqa(uint16_t * Atqa);
void MifareClassicSetAtqa(uint16_t Atqa);

void MifareClassicGetSak(uint8_t * Sak);
void MifareClassicSetSak(uint8_t Sak);

#endif /* MIFARECLASSIC_H_ */
