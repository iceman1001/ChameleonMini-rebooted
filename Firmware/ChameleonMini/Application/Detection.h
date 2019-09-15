#ifndef DETECTION_H
#define DETECTION_H

#include "Application.h"
#include "ISO14443-3A.h"

#define DETECTION_DATA_OFFSET           16
#define DETECTION_DATA_LEN              192

void MifareDetectionInit(void);
void MifareDetectionReset(void);
uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount);

#endif
