# This makefile uses sh in order to build the project.
# Note when using AVR-Toolchain, you have to install cygwin and
# append cygwin's bin directory to the PATH environment variable
# because Atmel does not ship sh.exe anymore with the toolchain.
SHELL = /bin/sh

# Switch to 'True' to tell if you work with an unlocked flashing setup (AVRISP mkII, or
# original Atmel bootloader), or 'False' else (original RevE rebooted and USB flashing).
# Original RevE rebooted needs specific scrambling to be flashed.
# If you don't know, do not touch!
UNLOCKED_F	 = False
# Python binary, needed to run scrambling operations to flash.
# Change it according to your system.
PYTHON_BIN	 = /usr/bin/env python3
# Tool for scrambling operations, to flash on locked (original bootloader) devices
CRYPTO_TOOL	 = ../../Software/Tools/crypt_operations.py

#Supported configurations
#WARNING. If you flash from Windows original tools, choose either CLASSIC OR ULTRALIGHT
#BOTH will produce a firmware that is too big for BOOT_LOADER_EXE to handle
SETTINGS	+= -DCONFIG_MF_CLASSIC_SUPPORT
#SETTINGS	+= -DCONFIG_MF_ULTRALIGHT_SUPPORT
SETTINGS	+= -DCONFIG_MF_CLASSIC_DETECTION_SUPPORT
#SETTINGS	+= -DCONFIG_MF_CLASSIC_BRUTE_SUPPORT
#SETTINGS	+= -DCONFIG_MF_CLASSIC_LOG_SUPPORT

#Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

#Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

#Enable the MEMORYTEST and/or MEMORYINFO commands to help with memory management debug
#REQUIRES enabling CONFIG_MF_CLASSIC_SUPPORT and CONFIG_MF_CLASSIC_DETECTION_SUPPORT
#SETTINGS    += -DCONFIG_DEBUG_MEMORYTEST_COMMAND
#SETTINGS    += -DCONFIG_DEBUG_MEMORYINFO_COMMAND

#Default configuration
SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_NONE

#Default button actions
SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS
#Default long button action
SETTINGS	+= -DDEFAULT_BUTTON_LONG_ACTION=BUTTON_ACTION_NONE

#Default setting
SETTINGS	+= -DDEFAULT_SETTING=0

#Default pending task timeout
SETTINGS	+= -DDEFAULT_PENDING_TASK_TIMEOUT=50 #* 100ms


#Build configuration
ifeq ($(OS),Windows_NT)
BUILD_DATE	 = "\"$(shell date /t)\""
else
BUILD_DATE	 = $(shell date +'\"%y%m%d\"')
endif
COMMIT_ID	 = $(shell git rev-parse --short HEAD)
MCU 		 = atxmega32a4u
ARCH		 = XMEGA
BOARD		 = NONE
F_CPU		 = 32000000
F_USB		 = 48000000
TARGET		 = ChameleonMini
OPTIMIZATION = s
SRC 		+= $(TARGET).c LUFADescriptors.c System.c Configuration.c Random.c Common.c Button.c Settings.c LED.c Map.c AntennaLevel.c
SRC 		+= Memory/EEPROM.c Memory/SPIFlash.c Memory/Memory.c
SRC 		+= Terminal/Terminal.c Terminal/Commands.c Terminal/XModem.c Terminal/CommandLine.c
SRC 		+= Codec/Codec.c Codec/ISO14443-2A.c
SRC 		+= Application/MifareUltralight.c Application/MifareClassic.c Application/ISO14443-3A.c Application/Crypto1.c
SRC 		+= $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH	 = ./LUFA
CC_FLAGS	 = -Werror -DUSE_LUFA_CONFIG_HEADER -DBUILD_DATE=$(BUILD_DATE) -DCOMMIT_ID=\"$(COMMIT_ID)\" $(SETTINGS)
OBJDIR		 = Bin

ifeq ($(UNLOCKED_F),True)
#AVRDUDE settings
AVRDUDE_PROGRAMMER = flip2
AVRDUDE_MCU = $(MCU)
AVRDUDE_PORT = usb
AVRDUDE_WRITE_APP = -U application:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

#AVRDUDE settings to program the precompiled firmware
AVRDUDE_WRITE_APP_LATEST = -U application:w:Latest/$(TARGET).hex
AVRDUDE_WRITE_EEPROM_LATEST = -U eeprom:w:Latest/$(TARGET).eep
endif

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
# include $(LUFA_PATH)/Build/lufa_doxygen.mk
# include $(LUFA_PATH)/Build/lufa_dfu.mk
# include $(LUFA_PATH)/Build/lufa_hid.mk
# include $(LUFA_PATH)/Build/lufa_avrdude.mk
# include $(LUFA_PATH)/Build/lufa_atprogram.mk

ifeq ($(UNLOCKED_F),True)
# Program the device using avrdude
program: $(TARGET).hex $(TARGET).eep
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP) $(AVRDUDE_WRITE_EEPROM)

# Program the device using avrdude with the latest official firmware
program-latest:
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP_LATEST) $(AVRDUDE_WRITE_EEPROM_LATEST)

# Program the device using batchisp and the DFU bootloader
# Note that the device has to be in bootloader mode already
dfu-flip: $(TARGET).hex $(TARGET).eep
	cp $(TARGET).eep EEPROM.hex
	batchisp -hardware usb -device $(MCU) -operation erase f memory FLASH loadbuffer $(TARGET).hex program verify memory EEPROM loadbuffer EEPROM.hex program verify start reset 0
	rm EEPROM.hex

# Program the device using dfu-programmer in unlocked flashing conditions
# Note that the device has to be in bootloader mode already
# dfu-programmer >= 0.7.2 required
dfu-prog: $(TARGET).hex $(TARGET).eep
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash --eeprom $(TARGET).eep
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) launch

else
# Program the device using dfu-programmer for unlocked RevE rebooted USB DFU scenario
# Note that the device has to be in bootloader mode already
# dfu-programmer >= 0.7.2 required
dfu-prog: $(TARGET).hex $(TARGET).eep
	$(PYTHON_BIN) $(CRYPTO_TOOL) scramblehex $(TARGET).eep $(TARGET).eep.scramble
	$(PYTHON_BIN) $(CRYPTO_TOOL) scramblehex $(TARGET).hex $(TARGET).hex.scramble
	-dfu-programmer $(MCU) erase --force
	dfu-programmer $(MCU) flash --eeprom --force --suppress-validation $(TARGET).eep.scramble
	dfu-programmer $(MCU) flash --force --suppress-validation $(TARGET).hex.scramble
	dfu-programmer $(MCU) launch
	rm $(TARGET).eep.scramble $(TARGET).hex.scramble
endif
