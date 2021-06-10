/*
 * NTAG21x.c
 *
 *  Created on: 20.02.2019
 *  Author: Giovanni Cammisa (gcammisa)
 *  Still missing support for:
 *      -The management of both static and dynamic lock bytes
 *      -Bruteforce protection (AUTHLIM COUNTER)
 *  Thanks to skuser for the MifareUltralight code used as a starting point
 */

#include "NTAG21x.h"
#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory/Memory.h"

//Writes a page
uint8_t AppWritePage(uint8_t PageAddress, uint8_t *const Buffer) {
    if (!ActiveConfiguration.ReadOnly) {
        AppCardMemoryWrite(Buffer, PageAddress * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);
    } else {
        /* If the chameleon is in read only mode, it silently
        * ignores any attempt to write data. */
    }
    return 0;
}

//HELPER FUNCTIONS
void NTAG21xGetUid(ConfigurationUidType Uid) {
    /* Read UID from memory */
    AppCardMemoryRead(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    AppCardMemoryRead(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
}

void NTAG21xSetUid(ConfigurationUidType Uid) {
    /* Calculate check bytes and write everything into memory */
    uint8_t BCC1 = ISO14443A_UID0_CT ^ Uid[0] ^ Uid[1] ^ Uid[2];
    uint8_t BCC2 = Uid[3] ^ Uid[4] ^ Uid[5] ^ Uid[6];

    AppCardMemoryWrite(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    AppCardMemoryWrite(&BCC1, UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
    AppCardMemoryWrite(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
    AppCardMemoryWrite(&BCC2, UID_BCC2_ADDRESS, ISO14443A_CL_BCC_SIZE);
}
