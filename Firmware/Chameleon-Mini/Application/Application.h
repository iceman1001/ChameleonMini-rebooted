/*
 * Application.h
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "../Common.h"
#include "../Configuration.h"

/* Applications */
#include "MifareClassic.h"
#include "MifareUltralight.h"
#include "Detection.h"

/* Function wrappers */
INLINE void ApplicationInit(void) {
    ActiveConfiguration.ApplicationInitFunc();
}

INLINE void ApplicationTask(void) {
    ActiveConfiguration.ApplicationTaskFunc();
}

INLINE void ApplicationTick(void) {
    ActiveConfiguration.ApplicationTickFunc();
}

INLINE uint16_t ApplicationProcess(uint8_t* ByteBuffer, uint16_t ByteCount) {
    return ActiveConfiguration.ApplicationProcessFunc(ByteBuffer, ByteCount);
}

INLINE void ApplicationReset(void) {
    ActiveConfiguration.ApplicationResetFunc();
}

INLINE void ApplicationGetUid(ConfigurationUidType Uid) {
	ActiveConfiguration.ApplicationGetUidFunc(Uid);
}

INLINE void ApplicationSetUid(ConfigurationUidType Uid) {
	ActiveConfiguration.ApplicationSetUidFunc(Uid);
	//LogEntry(LOG_INFO_UID_SET, Uid, ActiveConfiguration.UidSize);
}

INLINE void ApplicationGetSak(uint8_t * Sak) {
	ActiveConfiguration.ApplicationGetSakFunc(Sak);
}

INLINE void ApplicationSetSak(uint8_t Sak) {
	ActiveConfiguration.ApplicationSetSakFunc(Sak);
}

INLINE void ApplicationGetAtqa(uint16_t * Atqa) {
	ActiveConfiguration.ApplicationGetAtqaFunc(Atqa);
}

INLINE void ApplicationSetAtqa(uint16_t Atqa) {
	ActiveConfiguration.ApplicationSetAtqaFunc(Atqa);
}

#endif /* APPLICATION_H_ */
