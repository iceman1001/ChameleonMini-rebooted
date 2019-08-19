@ECHO OFF

echo.                
echo .---------------------------------------------------.
echo ^|                                                   ^|
echo ^|   ChameleonMini RevE Rebooted flasher utility     ^|
echo ^|                  Iceman fork                      ^|
echo ^|                                                   ^|
echo `---------------------------------------------------'
echo.

set dirpath=%~dp0
set oldname=Chameleon-Mini
set name=ChameleonMini

if not exist "%dirpath%BOOT_LOADER_EXE.exe" (
	echo Cannot find BOOT_LOADER_EXE.exe. Please run this .bat script in the same directory where it is saved.
	pause > nul
	exit
)

if not exist "%dirpath%avr-objcopy.exe" (
	echo Cannot find avr-objcopy.exe. Please run this .bat script in the same directory where it is saved.
	pause > nul
	exit
)

if not exist "%dirpath%Createbin.exe" (
	echo Cannot find Createbin.exe. Please run this .bat script in the same directory where it is saved.
	pause > nul
	exit
)

if not exist "%dirpath%%name%.eep" (
	if exist "%dirpath%%oldname%.eep" (
		move /y "%dirpath%%oldname%.eep" "%dirpath%%name%.eep"
	) else (
		echo Cannot find eep file. Please run this .bat script in the same directory where it is saved.
		pause > nul
		exit
	)
)

if not exist "%dirpath%%name%.hex" (
	if exist "%dirpath%%oldname%.hex" (
		move /y "%dirpath%%oldname%.hex" "%dirpath%%name%.hex"
	) else (
		echo Cannot find hex file. Please run this .bat script in the same directory where it is saved.
		pause > nul
		exit
	)
)

echo Creating the EEPROM binary...
"%dirpath%avr-objcopy.exe" -I ihex %name%.eep -O binary eeprom.bin

"%dirpath%Createbin.exe" eeprom.bin bin

del "%dirpath%eeprom.bin"

echo.
echo.

move "%dirpath%myfile.bin" "%dirpath%myfilee.bin" >nul

echo Creating the Flash binary...
"%dirpath%avr-objcopy.exe" -I ihex %name%.hex -O binary flash.bin

"%dirpath%Createbin.exe" flash.bin bin

del "%dirpath%flash.bin"

echo.
echo.

echo Flashing the files onto the "ChameleonMini RevE Rebooted"...
"%dirpath%BOOT_LOADER_EXE.exe"

echo.

echo If there are no errors above, flashing the firmware should be finished now. Enjoy!

