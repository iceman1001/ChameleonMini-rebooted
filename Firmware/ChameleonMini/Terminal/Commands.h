/* Copyright 2013 Timo Kasper, Simon KŸppers, David Oswald ("ORIGINAL
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

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "../Common.h"

#define MAX_COMMAND_LENGTH              16 // If this is more than 16 bytes, my unit becomes unresponsive with 8th entry.
#define MAX_STATUS_LENGTH               32

// Firmware version details.
#define FIRMWARE_VERSION_MAJOR          1
#define FIRMWARE_VERSION_MINOR          3
#define FIRMWARE_NAME                   "rebooted"
#define FIRMWARE_FORK_AUTHOR            "Iceman"
#ifdef  COMMIT_ID
#define FIRMWARE_COMMIT_STRING          COMMIT_ID
#else
#define FIRMWARE_COMMIT_STRING          "N/A"
#endif

#define COMMAND_INFO_OK_ID              100
#define COMMAND_INFO_OK                 "OK"
#define COMMAND_INFO_OK_WITH_TEXT_ID    101
#define COMMAND_INFO_OK_WITH_TEXT       "OK WITH TEXT"
#define COMMAND_INFO_XMODEM_WAIT_ID     110
#define COMMAND_INFO_XMODEM_WAIT        "WAITING FOR XMODEM"
#define COMMAND_ERR_UNKNOWN_CMD_ID      200
#define COMMAND_ERR_UNKNOWN_CMD         "UNKNOWN COMMAND"
#define COMMAND_ERR_INVALID_USAGE_ID    201
#define COMMAND_ERR_INVALID_USAGE       "INVALID COMMAND USAGE"
#define COMMAND_ERR_INVALID_PARAM_ID    202
#define COMMAND_ERR_INVALID_PARAM       "INVALID PARAMETER"
#define COMMAND_ERR_TIMEOUT_ID          203
#define COMMAND_ERR_TIMEOUT             "TIMEOUT"
#define TIMEOUT_COMMAND                 255 // this is just for the CommandLine module to know that this is a timeout command


#define COMMAND_CHAR_TRUE               '1'
#define COMMAND_CHAR_FALSE              '0'

#define COMMAND_UID_BUFSIZE             32

typedef uint8_t CommandStatusIdType;
typedef const char CommandStatusMessageType[MAX_STATUS_LENGTH];

typedef CommandStatusIdType (*CommandExecFuncType) (char* OutMessage);
typedef CommandStatusIdType (*CommandExecParamFuncType) (char* OutMessage, const char* InParams);
typedef CommandStatusIdType (*CommandSetFuncType) (char* OutMessage, const char* InParam);
typedef CommandStatusIdType (*CommandGetFuncType) (char* OutParam);

typedef struct {
  char Command[MAX_COMMAND_LENGTH];
  CommandExecFuncType ExecFunc;
  CommandExecParamFuncType ExecParamFunc;
  CommandSetFuncType SetFunc;
  CommandGetFuncType GetFunc;
} CommandEntryType;

#define COMMAND_VERSION             "VERSION"
CommandStatusIdType CommandGetVersion(char* OutParam);

#define COMMAND_CONFIG              "CONFIG"
CommandStatusIdType CommandExecConfig(char* OutMessage);
CommandStatusIdType CommandGetConfig(char* OutParam);
CommandStatusIdType CommandSetConfig(char* OutMessage, const char* InParam);

#define COMMAND_UID                 "UID"
#define COMMAND_UID_RANDOM          "RANDOM"
CommandStatusIdType CommandGetUid(char* OutParam);
CommandStatusIdType CommandSetUid(char* OutMessage, const char* InParam);

#define COMMAND_ATQA                "ATQA"
CommandStatusIdType CommandGetAtqa(char* OutParam);
CommandStatusIdType CommandSetAtqa(char* OutMessage, const char* InParam);

#define COMMAND_SAK                 "SAK"
CommandStatusIdType CommandGetSak(char* OutParam);
CommandStatusIdType CommandSetSak(char* OutMessage, const char* InParam);

#define COMMAND_READONLY            "READONLY"
CommandStatusIdType CommandGetReadOnly(char* OutParam);
CommandStatusIdType CommandSetReadOnly(char* OutMessage, const char* InParam);

#define COMMAND_UPLOAD              "UPLOAD"
CommandStatusIdType CommandExecUpload(char* OutMessage);

#define COMMAND_DOWNLOAD            "DOWNLOAD"
CommandStatusIdType CommandExecDownload(char* OutMessage);

#define COMMAND_RESET               "RESET"
CommandStatusIdType CommandExecReset(char* OutMessage);

#define COMMAND_UPGRADE             "UPGRADE"
CommandStatusIdType CommandExecUpgrade(char* OutMessage);

#define COMMAND_MEMSIZE             "MEMSIZE"
CommandStatusIdType CommandGetMemSize(char* OutParam);

#define COMMAND_WORKMEM             "WORKMEM"
CommandStatusIdType CommandGetWorkingMem(char* OutParam);
CommandStatusIdType CommandExecWorkingMem(char* OutMessage);

#define COMMAND_WORKMEMUPLOAD       "WORKMEMUPLOAD"
CommandStatusIdType CommandExecWorkingMemUpload(char* OutMessage);

#define COMMAND_WORKMEMDOWNLOAD     "WORKMEMDOWNLOAD"
CommandStatusIdType CommandExecWorkingMemDownload(char* OutMessage);

#define COMMAND_UIDSIZE             "UIDSIZE"
CommandStatusIdType CommandGetUidSize(char* OutParam);

#define COMMAND_BUTTON              "BUTTON"
CommandStatusIdType CommandExecButton(char* OutMessage);
CommandStatusIdType CommandGetButton(char* OutParam);
CommandStatusIdType CommandSetButton(char* OutMessage, const char* InParam);

#define COMMAND_BUTTON_LONG         "BUTTON_LONG"
CommandStatusIdType CommandExecButtonLong(char* OutMessage);
CommandStatusIdType CommandGetButtonLong(char* OutParam);
CommandStatusIdType CommandSetButtonLong(char* OutMessage, const char* InParam);

#define COMMAND_SETTING             "SETTING"
CommandStatusIdType CommandGetSetting(char* OutParam);
CommandStatusIdType CommandSetSetting(char* OutMessage, const char* InParam);

#define COMMAND_CLEAR               "CLEAR"
CommandStatusIdType CommandExecClear(char* OutParam);

#define COMMAND_HELP                "HELP"
CommandStatusIdType CommandExecHelp(char* OutMessage);

#define COMMAND_RSSI                "RSSI"
CommandStatusIdType CommandGetRssi(char* OutParam);

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
#define COMMAND_PWD                 "PWD"
CommandStatusIdType CommandGetUltralightPassword(char* OutParam);
#endif

#ifdef CONFIG_MF_CLASSIC_DETECTION_SUPPORT
#define COMMAND_DETECTION           "DETECTION"
CommandStatusIdType CommandGetDetection(char* OutParam);
#endif

#define COMMAND_CLEARALL            "CLEARALL"
CommandStatusIdType CommandExecClearAll(char* OutMessage);

#ifdef CONFIG_DEBUG_MEMORYINFO_COMMAND
#define COMMAND_MEMORYINFO          "MEMORYINFO"
CommandStatusIdType CommandExecMemoryInfo(char* OutMessage);
#endif

#ifdef CONFIG_DEBUG_MEMORYTEST_COMMAND
#define COMMAND_MEMORYTEST          "MEMORYTEST"
CommandStatusIdType CommandExecMemoryTest(char* OutMessage);
#endif

#define COMMAND_LIST_END            ""

/* Defines the end of command list. This is no actual command */

#endif /* COMMANDS_H_ */
