/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#include "Commands.h"
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "XModem.h"
#include "../Settings.h"
#include "../ChameleonMini.h"
#include "../LUFA/Version.h"
#include "../Configuration.h"
#include "../Random.h"
#include "../Memory/Memory.h"
#if defined(CONFIG_DEBUG_MEMORYINFO_COMMAND) || defined(CONFIG_DEBUG_MEMORYTEST_COMMAND)
#include "../Memory/SPIFlash.h"
#include "../Memory/EEPROM.h"
#endif
#include "../System.h"
#include "../Button.h"
#include "../AntennaLevel.h"

extern const PROGMEM CommandEntryType CommandTable[];

CommandStatusIdType CommandGetVersion(char* OutParam) {
    snprintf_P( OutParam, TERMINAL_BUFFER_SIZE, PSTR("ChameleonMini-%s v%d.%d (%s: %s)"),
                FIRMWARE_NAME, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR,
                FIRMWARE_FORK_AUTHOR, FIRMWARE_COMMIT_STRING );
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetConfig(char* OutParam) {
    ConfigurationGetByName(OutParam, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetConfig(char* OutMessage, const char* InParam) {
    if (ConfigurationSetByName(InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecConfig(char* OutMessage) {
  ConfigurationGetList(OutMessage, TERMINAL_BUFFER_SIZE);
  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetAtqa(char* OutParam) {
    uint16_t Atqa;

    ApplicationGetAtqa(&Atqa);

    // Convert uint16 to uint8 buffer[]
    uint8_t atqaBuffer[2] = { 0,0 };
    atqaBuffer[1] = (uint8_t)Atqa;
    atqaBuffer[0] = Atqa >> 8;

    BufferToHexString(OutParam, TERMINAL_BUFFER_SIZE, &atqaBuffer, sizeof(uint16_t));

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetAtqa(char* OutMessage, const char* InParam) {
    uint8_t AtqaBuffer[2] = { 0, 0 };
    uint16_t Atqa = 0;

    if (HexStringToBuffer(&AtqaBuffer, sizeof(AtqaBuffer), InParam) != sizeof(uint16_t)) {
        // This has to be 4 digits (2 bytes), e.g.: 0004
        return COMMAND_ERR_INVALID_PARAM_ID;
    }

    // Convert uint8 buffer[] to uint16
    if (strlen(InParam) > 2) {
        Atqa = ((uint16_t)AtqaBuffer[0] << 8) | AtqaBuffer[1];
    }
    else {
        Atqa = AtqaBuffer[0];
    }

    ApplicationSetAtqa(Atqa);
    return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetSak(char* OutParam) {
    uint8_t Sak;

    ApplicationGetSak(&Sak);

    BufferToHexString(OutParam, TERMINAL_BUFFER_SIZE, &Sak, sizeof(uint8_t));
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetSak(char* OutMessage, const char* InParam) {
    uint8_t Sak;

    if (HexStringToBuffer(&Sak, sizeof(uint8_t), InParam) != sizeof(uint8_t)) {
        // This has to be 2 digits (1 byte), e.g.: 04
        return COMMAND_ERR_INVALID_PARAM_ID;
    }

    ApplicationSetSak(Sak);
    return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetUid(char* OutParam) {
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  ApplicationGetUid(UidBuffer);
  BufferToHexString(OutParam, TERMINAL_BUFFER_SIZE, UidBuffer, UidSize);
  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetUid(char* OutMessage, const char* InParam) {
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  if (strcmp_P(InParam, PSTR(COMMAND_UID_RANDOM)) == 0) {
    /* Load with random bytes */
    for (uint8_t i=0; i<UidSize; i++) {
      UidBuffer[i] = RandomGetByte();
    }
  } else {
    /* Convert to Bytes */
    if (HexStringToBuffer(UidBuffer, sizeof(UidBuffer), InParam) != UidSize) {
      /* Malformed input. Abort */
      return COMMAND_ERR_INVALID_PARAM_ID;
    }
  }

  ApplicationSetUid(UidBuffer);
  return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetReadOnly(char* OutParam)
{
  if (ActiveConfiguration.ReadOnly) {
    OutParam[0] = COMMAND_CHAR_TRUE;
  } else {
    OutParam[0] = COMMAND_CHAR_FALSE;
  }

  OutParam[1] = '\0';

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetReadOnly(char* OutMessage, const char* InParam)
{
  if (InParam[1] == '\0') {
    if (InParam[0] == COMMAND_CHAR_TRUE) {
      ActiveConfiguration.ReadOnly = true;
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_FALSE) {
      ActiveConfiguration.ReadOnly = false;
      return COMMAND_INFO_OK_ID;
    }
  }

  return COMMAND_ERR_INVALID_PARAM_ID;
}

CommandStatusIdType CommandExecUpload(char* OutMessage) {
    XModemReceive(AppCardMemoryUploadXModem);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecDownload(char* OutMessage) {
    XModemSend(AppCardMemoryDownloadXModem);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecReset(char* OutMessage) {
    USB_Detach();
    USB_Disable();
    SystemReset();
    return COMMAND_INFO_OK_ID;
}

#ifdef SUPPORT_FIRMWARE_UPGRADE
CommandStatusIdType CommandExecUpgrade(char* OutMessage) {
    USB_Detach();
    USB_Disable();
    SystemEnterBootloader();
    return COMMAND_INFO_OK_ID;
}
#endif

CommandStatusIdType CommandGetMemSize(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%lu"), ActiveConfiguration.CardMemorySize);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

void _readAndSendWorkingMemChunk(uint32_t Address, uint16_t Size, uint16_t MaxSize, bool isHex) {
    if((MaxSize <= TERMINAL_BUFFER_SIZE) && (Size <= MaxSize)) {
        uint8_t buff[Size];
        AppWorkingMemoryRead(buff, Address, Size);
        if(isHex) {
            char hexbuff[Size*2+3];
            uint16_t zeroIndex = Size*2;
            BufferToHexString(hexbuff, Size*2+3, buff, Size);
            hexbuff[zeroIndex] = '\r';
            hexbuff[zeroIndex+1] = '\n';
            hexbuff[zeroIndex+2] = '\0';
            TerminalSendString(hexbuff);
        } else {
            TerminalSendBlock(buff, Size);
        }
    }
}

uint8_t _readAndSendWorkingMem(uint16_t MaxSize, bool isHex) {
    uint8_t ret = COMMAND_ERR_INVALID_USAGE_ID;
    uint32_t bytesToWrite = AppWorkingMemorySize();
    if( (MaxSize <= TERMINAL_BUFFER_SIZE) && bytesToWrite ) {
        uint32_t rounds = bytesToWrite / MaxSize;
        uint16_t offset = bytesToWrite % MaxSize;
        for(uint32_t i=0; i < rounds; i++) {
            _readAndSendWorkingMemChunk(i*MaxSize, MaxSize, MaxSize, isHex);
        }
        if(offset) {
            _readAndSendWorkingMemChunk(rounds*MaxSize, offset, MaxSize, isHex);
        }
        ret = (isHex) ? (COMMAND_INFO_OK_WITH_TEXT_ID) : (COMMAND_INFO_OK_ID);
    }
    return ret;
}

CommandStatusIdType CommandGetWorkingMem(char* OutParam) {
    return _readAndSendWorkingMem(16, true);
}

CommandStatusIdType CommandExecWorkingMem(char* OutMessage) {
    return _readAndSendWorkingMem(TERMINAL_BUFFER_SIZE, false);
}

CommandStatusIdType CommandExecWorkingMemUpload(char* OutMessage) {
    XModemReceive(AppWorkingMemoryUploadXModem);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecWorkingMemDownload(char* OutMessage) {
    XModemSend(AppWorkingMemoryDownloadXModem);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandGetUidSize(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), ActiveConfiguration.UidSize);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandExecButton(char* OutMessage)
{
    ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButton(char* OutParam)
{
    ButtonGetActionByName(BUTTON_PRESS_SHORT, OutParam, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButton(char* OutMessage, const char* InParam)
{
    if (ButtonSetActionByName(BUTTON_PRESS_SHORT, InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecButtonLong(char* OutMessage)
{
    ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButtonLong(char* OutParam)
{
    ButtonGetActionByName(BUTTON_PRESS_LONG, OutParam, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButtonLong(char* OutMessage, const char* InParam)
{
    if (ButtonSetActionByName(BUTTON_PRESS_LONG, InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetSetting(char* OutParam) {
    SettingsGetActiveByName(OutParam, TERMINAL_BUFFER_SIZE);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetSetting(char* OutMessage, const char* InParam) {
    if (SettingsSetActiveByName(InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecClear(char* OutParam) {
    AppMemoryClear();
    ConfigurationSetById(DEFAULT_CONFIGURATION);
    ButtonSetActionById(BUTTON_PRESS_SHORT, DEFAULT_BUTTON_ACTION);
    ButtonSetActionById(BUTTON_PRESS_LONG, DEFAULT_BUTTON_LONG_ACTION);
    SettingsSave();
    return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandExecHelp(char* OutMessage) {
    const CommandEntryType* EntryPtr = CommandTable;
    uint16_t ByteCount = TERMINAL_BUFFER_SIZE - 1; /* Account for '\0' */

    while(strcmp_P(COMMAND_LIST_END, EntryPtr->Command) != 0) {
        const char* CommandName = EntryPtr->Command;
        char c;

        while( (c = pgm_read_byte(CommandName)) != '\0' && ByteCount > 32) {
            *OutMessage++ = c;
            CommandName++;
            ByteCount--;
        }

        *OutMessage++ = ',';
        ByteCount--;

        EntryPtr++;
    }

    *--OutMessage = '\0';

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetRssi(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,  PSTR("%5u mV"), AntennaLevelGet());
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
CommandStatusIdType CommandGetUltralightPassword(char* OutParam) {
    uint8_t pwd[MIFARE_ULTRALIGHT_PWD_SIZE];
    /* Read saved password from authentication */
    AppWorkingMemoryRead(pwd, MIFARE_ULTRALIGHT_PWD_ADDRESS, MIFARE_ULTRALIGHT_PWD_SIZE);
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,  PSTR("%02x%02x%02x%02x"), pwd[0], pwd[1], pwd[2], pwd[3]);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}
#endif

#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
CommandStatusIdType CommandGetDetection(char* OutParam) {
    /* Read UID / s0-b0 */
    AppWorkingMemoryRead(OutParam, MFCLASSIC_MEM_S0B0_ADDRESS, DETECTION_MEM_BLOCK0_SIZE);
    /* Read saved nonce data from authentication */
    AppWorkingMemoryRead(OutParam+DETECTION_MEM_BLOCK0_SIZE, DETECTION_MEM_DATA_START_ADDR, DETECTION_MEM_MFKEY_DATA_LEN);
    /* Add file integrity to byte. This adds 2 bytes (209, 210) to DETECTION_MEM_APP_SIZE */
    ISO14443AAppendCRCA(OutParam, DETECTION_MEM_APP_SIZE);
    /* Send data + CRC */
    for(uint8_t num=0; num < DETECTION_MEM_APP_SIZE+2; num++) {
       TerminalSendChar(OutParam[num]);
    }
    OutParam[0] = 0;
    return COMMAND_INFO_OK_ID;
}
#endif

#ifdef SUPPORT_MF_CLASSIC_MAGIC_MODE
CommandStatusIdType CommandGetUidMode(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), GlobalSettings.UidMode);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}
CommandStatusIdType CommandSetUidMode(char* OutMessage, const char* InParam) {
    if (InParam[1] == '\0') {
    if (InParam[0] == COMMAND_CHAR_TRUE) {
      SettingsSetUidMode(true);
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_FALSE) {
      SettingsSetUidMode(false);
      return COMMAND_INFO_OK_ID;
    }
  }
  return COMMAND_ERR_INVALID_PARAM_ID;
}
#endif

CommandStatusIdType CommandExecClearAll(char* OutMessage) {
    MemoryClearAll();
    for(uint8_t i = SETTINGS_FIRST; i <= SETTINGS_LAST; i++) {
        SettingsSetActiveById(i);
        ConfigurationSetById(DEFAULT_CONFIGURATION);
        ButtonSetActionById(BUTTON_PRESS_SHORT, DEFAULT_BUTTON_ACTION);
        ButtonSetActionById(BUTTON_PRESS_LONG, DEFAULT_BUTTON_LONG_ACTION);
    }
    SettingsSetActiveById(DEFAULT_SETTING);
    SettingsSave();
    return COMMAND_INFO_OK_ID;
}

#ifdef CONFIG_DEBUG_MEMORYINFO_COMMAND
CommandStatusIdType CommandExecMemoryInfo(char* OutMessage) {
    snprintf_P( OutMessage, TERMINAL_BUFFER_SIZE,
        PSTR("SPI Flash:\r\n- Bytes Per Setting: %lu\r\n- Bytes Per Card Memory: %lu\r\n- MDID Bytes: %02X%02X%02X%02X\r\n- Memory size: %u Mbits (%u KBytes)\r\nEEPROM:\r\n- Bytes Per Setting: %u\r\n- Memory size: %u Bytes"),
        MemoryMappingInfo.maxFlashBytesPerSlot, MemoryMappingInfo.maxFlashBytesPerCardMemory,
        FlashInfo.manufacturerId, FlashInfo.deviceId1, FlashInfo.deviceId2, FlashInfo.edi,
        FlashInfo.geometry.sizeMbits, FlashInfo.geometry.sizeKbytes,
        MemoryMappingInfo.maxEEPROMBytesPerSlot, EEPROMInfo.bytesTotal );
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}
#endif

#ifdef CONFIG_DEBUG_MEMORYTEST_COMMAND
CommandStatusIdType CommandExecMemoryTest(char* OutMessage) {
    uint8_t bigbuf[128];
    uint8_t readbuf[50];
    uint8_t expected[50];
    bool retValFail = false;
    bool retValOK = false;
    bool tempRetVal = false;
    HexStringToBuffer(expected, 50,
        "000111FFAA031111111111111111111100FFFFFFFFFF03FFFFFFFFFFFFFFFFFFFF00FFFFFFFFFFFFFFFFFFFF05111111AA02");
    memset(bigbuf, 0x11, 128);
    memset(readbuf, 0xAA, 50);

    SettingsSetActiveById(3);
    ConfigurationSetById(CONFIG_MF_CLASSIC_1K);
    SettingsSave();
    SettingsSetActiveById(2);
    ConfigurationSetById(CONFIG_MF_CLASSIC_DETECTION);
    SettingsSave();
    SettingsSetActiveById(0);
    ConfigurationSetById(CONFIG_MF_CLASSIC_4K);
    SettingsSave();

    tempRetVal = AppCardMemoryWriteForSetting(3, bigbuf, 0, 128);
    retValOK = tempRetVal;
    for(uint8_t i = 0; i < 32; i++){
         tempRetVal = AppCardMemoryWrite(bigbuf, i*128, 128);
         retValOK = (retValOK && tempRetVal);
    }
    tempRetVal = FlashUnbufferedBytesRead(readbuf+2, 3*MemoryMappingInfo.maxFlashBytesPerSlot, 1);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppCardMemoryReadForSetting(3, readbuf+3, 128, 1);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppCardMemoryReadForSetting(3, readbuf, MFCLASSIC_1K_MEM_SIZE, 4);
    retValFail = tempRetVal;
    tempRetVal = AppWorkingMemoryReadForSetting(3, readbuf, 0, 1);
    retValFail = (retValFail || tempRetVal);
    readbuf[5] = 0x03;
    tempRetVal = FlashUnbufferedBytesRead(readbuf+6, 1024, 1);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = FlashUnbufferedBytesRead(readbuf+7, 3118, 2);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppCardMemoryRead(readbuf+9, 2046, 3);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppCardMemoryRead(readbuf+12, 12, 4);
    retValOK = (retValOK && tempRetVal);
    readbuf[16] = 0x00;
    tempRetVal = FlashClearRange(3*MemoryMappingInfo.maxFlashBytesPerSlot, 16);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppMemoryClear();
    retValOK = (retValOK && tempRetVal);
    FlashUnbufferedBytesRead(readbuf+17, 3*MemoryMappingInfo.maxFlashBytesPerSlot, 4);
    AppCardMemoryReadForSetting(3, readbuf+21, 128, 1);
    readbuf[22] = 0x03;
    FlashUnbufferedBytesRead(readbuf+23, 1024, 1);
    FlashUnbufferedBytesRead(readbuf+24, 3118, 2);
    AppCardMemoryRead(readbuf+26, 2046, 3);
    AppCardMemoryRead(readbuf+29, 12, 4);
    readbuf[33] = 0x00;
    FlashUnbufferedBytesRead(readbuf+34, 5*MemoryMappingInfo.maxFlashBytesPerSlot+11, 10);
    readbuf[44] = 0x05;
    tempRetVal = AppWorkingMemoryWriteForSetting(2, bigbuf, 0, 16);
    retValOK = (retValOK && tempRetVal);
    tempRetVal = AppWorkingMemoryReadForSetting(2, readbuf+45, 5, 3);
    retValOK = (retValOK && tempRetVal);
    readbuf[49] = 0x02;
    readbuf[0] = retValFail;
    readbuf[1] = retValOK;
    MemoryClearAll();

    if(memcmp(readbuf, expected, 50)) {
        BufferToHexString(OutMessage, TERMINAL_BUFFER_SIZE, readbuf, 50);
    } else {
        snprintf_P(OutMessage, TERMINAL_BUFFER_SIZE,  PSTR("FINE"), NULL);
    }

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}
#endif
