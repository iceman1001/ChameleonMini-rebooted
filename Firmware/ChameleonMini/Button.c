#include "Button.h"
#include "Random.h"
#include "Configuration.h"
#include "Common.h"
#include "Settings.h"

static const char PROGMEM ButtonActionTable[][BUTTON_NAME_MAX_LEN] =
{
    [BUTTON_ACTION_NONE] = "CLOSED",
    [BUTTON_ACTION_UID_RANDOM] = "RANDOM_UID",
    [BUTTON_ACTION_UID_LEFT_INCREMENT] = "UID_LEFT_INCREMENT",
    [BUTTON_ACTION_UID_RIGHT_INCREMENT] = "UID_RIGHT_INCREMENT",
    [BUTTON_ACTION_UID_LEFT_DECREMENT] = "UID_LEFT_DECREMENT",
    [BUTTON_ACTION_UID_RIGHT_DECREMENT] = "UID_RIGHT_DECREMENT",
    [BUTTON_ACTION_CYCLE_SETTINGS] = "SWITCHCARD",
    [BUTTON_ACTION_TOGGLE_READONLY] = "READONLY",
    [BUTTON_ACTION_FUNCTION] = "CARD_FUNCTION",
};

void ButtonInit(void)
{
    BUTTON_PORT.DIRCLR = BUTTON_MASK;
    BUTTON_PORT.BUTTON_PINCTRL = PORT_OPC_PULLUP_gc;
}

static void ExecuteButtonAction(ButtonActionEnum ButtonAction)
{
    ConfigurationUidType UidBuffer;

    if (ButtonAction == BUTTON_ACTION_UID_RANDOM) {
        uint8_t startByte = 0;
        ApplicationGetUid(UidBuffer);

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
        // Make RANDOM keep 1st byte safe for Ultralight types
        ConfigurationEnum ActiveConfigurationId = GlobalSettings.ActiveSettingPtr->Configuration;
        if( (ActiveConfigurationId == CONFIG_MF_ULTRALIGHT)
            || (ActiveConfigurationId == CONFIG_MF_ULTRALIGHT_EV1_80B)
            || (ActiveConfigurationId == CONFIG_MF_ULTRALIGHT_EV1_164B) ) {
            startByte = 1;
        }
#endif
        for( ; startByte < ActiveConfiguration.UidSize; startByte++) {
            UidBuffer[startByte] = RandomGetByte();
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
    } else if (ButtonAction == BUTTON_ACTION_TOGGLE_READONLY) {
        ActiveConfiguration.ReadOnly = !ActiveConfiguration.ReadOnly;
    } else if (ButtonAction == BUTTON_ACTION_FUNCTION) {
        ApplicationButton();
    }
}

void ButtonTick(void)
{
    //static uint8_t LastButtonState = 0;
    static uint8_t PressTickCounter = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;
    //uint8_t ThisButtonChange = ThisButtonState ^ LastButtonState;
    //uint8_t ThisButtonPress = ThisButtonChange & ThisButtonState;
    //LastButtonState = ThisButtonState;

    if (ThisButtonState & BUTTON_MASK) {
        /* Button is currently pressed */
        if (PressTickCounter < BUTTON_LONG_PRESS_TICK_COUNT) {
            /* Count ticks while button is being pressed */
            PressTickCounter++;
        } else if (PressTickCounter == BUTTON_LONG_PRESS_TICK_COUNT) {
            /* Long button press detected execute button action and advance PressTickCounter
             * to an invalid state. */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonLongAction);
            PressTickCounter++;
        } else {
            /* Button is still pressed, ignore */
        }
    } else if (!(ThisButtonState & BUTTON_MASK)) {
        /* Button is currently not being pressed. Check if PressTickCounter contains
         * a recent short button press. */
        if ( (PressTickCounter > 0) && (PressTickCounter <= BUTTON_LONG_PRESS_TICK_COUNT) ) {
            /* We have a short button press */
            ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonAction);
        }

        PressTickCounter = 0;
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

void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action)
{
    #ifndef BUTTON_SETTING_GLOBAL
    if (Type == BUTTON_PRESS_SHORT) {
        GlobalSettings.ActiveSettingPtr->ButtonAction = Action;
        } else if (Type == BUTTON_PRESS_LONG) {
        GlobalSettings.ActiveSettingPtr->ButtonLongAction = Action;
    }
    #else
    /* Write button action to all settings when using global settings */
    for (uint8_t i=0; i<SETTINGS_COUNT; i++) {
        if (Type == BUTTON_PRESS_SHORT) {
            GlobalSettings.Settings[i].ButtonAction = Action;
            } else if (Type == BUTTON_PRESS_LONG) {
            GlobalSettings.Settings[i].ButtonLongAction = Action;
        }
    }
    #endif
}

void ButtonGetActionByName(ButtonTypeEnum Type, char* ActionOut, uint16_t BufferSize)
{
    if (Type == BUTTON_PRESS_SHORT) {
        strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonAction], BufferSize);
        } else if (Type == BUTTON_PRESS_LONG) {
        strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonLongAction], BufferSize);
        } else {
        /* Should not happen (TM) */
        *ActionOut = '\0';
    }
}

bool ButtonSetActionByName(ButtonTypeEnum Type, const char* Action)
{
    uint8_t i;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        if (strcmp_P(Action, ButtonActionTable[i]) == 0) {
            ButtonSetActionById(Type, i);
            return true;
        }
    }

    /* Button action not found */
    return false;
}
