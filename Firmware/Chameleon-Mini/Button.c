#include "Button.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"

static const char PROGMEM ButtonActionTable[][32] =
{
    [BUTTON_ACTION_NONE] = "CLOSED",
    [BUTTON_ACTION_UID_RANDOM] = "RANDOM_UID",
    [BUTTON_ACTION_UID_LEFT_INCREMENT] = "UID_LEFT_INCREMENT",
    [BUTTON_ACTION_UID_RIGHT_INCREMENT] = "UID_RIGHT_INCREMENT",
    [BUTTON_ACTION_UID_LEFT_DECREMENT] = "UID_LEFT_DECREMENT",
    [BUTTON_ACTION_UID_RIGHT_DECREMENT] = "UID_RIGHT_DECREMENT",
    [BUTTON_ACTION_CYCLE_SETTINGS] = "SWITCHCARD"
};

void ButtonInit(void)
{
	BUTTON_PORT.DIRCLR = BUTTON_MASK;
	BUTTON_PORT.BUTTON_PINCTRL = PORT_OPC_PULLUP_gc;
}

void ButtonTick(void)
{
    static uint8_t LastButtonState = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;
    uint8_t ThisButtonChange = ThisButtonState ^ LastButtonState;
    uint8_t ThisButtonPress = ThisButtonChange & ThisButtonState;
    LastButtonState = ThisButtonState;

    if ( ThisButtonPress & BUTTON_MASK ) {
        uint8_t UidBuffer[32];
        ButtonActionEnum ButtonAction = GlobalSettings.ActiveSettingPtr->ButtonAction;

        if (ButtonAction == BUTTON_ACTION_UID_RANDOM) {
            for (uint8_t i=0; i<ActiveConfiguration.UidSize; i++) {
                UidBuffer[i] = RandomGetByte();
            }

            ApplicationSetUid(UidBuffer);
        } else if (ButtonAction == BUTTON_ACTION_UID_LEFT_INCREMENT) {
            ApplicationGetUid(UidBuffer);
            bool Carry = 1;
            uint8_t i;

            for (i=0; i<ActiveConfiguration.UidSize; i++) {
                if (Carry) {
                    if (UidBuffer[i] == 0xFF) {
                        Carry = 1;
                    } else {
                        Carry = 0;
                    }

                    UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
                }
            }

            ApplicationSetUid(UidBuffer);
        } else if (ButtonAction == BUTTON_ACTION_UID_RIGHT_INCREMENT) {
            ApplicationGetUid(UidBuffer);
            bool Carry = 1;
            uint8_t i = ActiveConfiguration.UidSize;

            while(i-- > 0) {
                if (Carry) {
                    if (UidBuffer[i] == 0xFF) {
                        Carry = 1;
                    } else {
                        Carry = 0;
                    }

                    UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
                }
            }

            ApplicationSetUid(UidBuffer);
        } else if (ButtonAction == BUTTON_ACTION_UID_LEFT_DECREMENT) {
            ApplicationGetUid(UidBuffer);
            bool Carry = 1;
            uint8_t i;

            for (i=0; i<ActiveConfiguration.UidSize; i++) {
                if (Carry) {
                    if (UidBuffer[i] == 0x00) {
                        Carry = 1;
                    } else {
                        Carry = 0;
                    }

                    UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
                }
            }

            ApplicationSetUid(UidBuffer);
        } else if (ButtonAction == BUTTON_ACTION_UID_RIGHT_DECREMENT) {
            ApplicationGetUid(UidBuffer);
            bool Carry = 1;
            uint8_t i = ActiveConfiguration.UidSize;

            while(i-- > 0) {
                if (Carry) {
                    if (UidBuffer[i] == 0x00) {
                        Carry = 1;
                    } else {
                        Carry = 0;
                    }

                    UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
                }
            }

            ApplicationSetUid(UidBuffer);
        } else if (ButtonAction == BUTTON_ACTION_CYCLE_SETTINGS) {
        	SettingsCycle();
			SettingsSave();
        }
    }
}

void ButtonGetActionList(char* ListOut, uint16_t BufferSize)
{
    uint8_t i;

    /* Account for '\0' */
    BufferSize--;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        const char* ActionName = ButtonActionTable[i];
        char c;

        while( (c = pgm_read_byte(ActionName)) != '\0' && BufferSize > sizeof(ButtonActionTable[i]) ) {
            /* While not end-of-string and enough buffer to
            * put a complete configuration name */
            *ListOut++ = c;
            ActionName++;
            BufferSize--;
        }

        if ( i < (BUTTON_ACTION_COUNT - 1) ) {
            /* No comma on last configuration */
            *ListOut++ = ',';
            BufferSize--;
        }
    }

    *ListOut = '\0';
}

void ButtonSetActionById(ButtonActionEnum Action)
{
	GlobalSettings.ActiveSettingPtr->ButtonAction = Action;
}

void ButtonGetActionByName(char* ActionOut, uint16_t BufferSize)
{
    strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonAction], BufferSize);
}

bool ButtonSetActionByName(const char* Action)
{
    uint8_t i;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        if (strcmp_P(Action, ButtonActionTable[i]) == 0) {
            ButtonSetActionById(i);
            return true;
        }
    }

    /* Button action not found */
    return false;
}
