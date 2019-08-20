---
name: 'Bug Report'
about: 'Something did not function as expected, and you would like to help fixing it.'
title: '[BUG] Please enter a short description'
labels: 'bug'
assignees: ''

---
<!-- Everything wrote in between such markers before and after this phrase are comments, will not be displayed, and are to be replaced or can be deleted. The rest is to let untouched, except where specified, or your report will be ugly. Use "Preview" tab just above to check how things will be displayed. -->
<!-- If you do not understand what is requested somewhere in here, please read Wiki pages and read other issues first. -->
## Environment
<!-- Write your information on the right column between the | | characters. Do not delete/modify any | character. If you need to use a | character in your writing, or code, put them between back-quotes, `like | this`. -->
|Item|Your information|
|---|---|
|**Harware**|<!-- Version of your hardware: RevE Rebooted, RevG, Other, I don't known. If you don't know, it's probably written on it under the Chameleon picture. As a reminder, this repositoty only supports RevE Rebooted for now -->
|**Firmware**|<!-- Write the full version name of your firmware. Use the "VERSION" command, or look at "Settings" tab in GUI. -->|
|**GUI**|<!-- Put a URL to file or repository source whre you found the GUI you are using to program or configure your ChameleonMini. If you do not use one and use commands only, type Terminal. Firmware in this repo is fully supported by rebootedGUI only. -->|
|**Slot number**|<!-- Put the slot number you are using when the bug appears, from 1 to 8. If it is not applicable, put N/A. -->|
|**Slot configuration**|<!-- Write the setting name of the slot you are using when the bug appears. The setting name might be one like "MF_CLASSIC_1K" or "MF_DETECTION", amongst the list found on wiki here: https://github.com/iceman1001/ChameleonMini-rebooted/wiki/Configurations. If it is not applicable, put N/A. -->|
|**Dump source**|<!-- If you are using a card dump on the slot when the bug appears, briefly describe where your dump come from (card source, how did you dump it). If it is not applicable, put N/A. -->|
|**Reader**|<!-- Briefly describe the reader or device to which you applied your ChameleonMini (brand, type, use case). If it is not applicable, put N/A. -->|
|**Flashing environment**|<!-- Briefly describe the environment from which you compile, flash and program your ChameleonMini (OS, version). If you use different ones from different activities, or could reproduce the bug on different environments, describe them all. If it is not applicable, put N/A. -->|
|**Flashing method**|<!-- How do you connect and flash your ChameleonMini to your computer to flash it. Please specify amongst: USB, AVRISPmkII, dfu-programmer, flash.bat, BOOT_LOADER_EXE.exe, other (describe). If it is not applicable, put N/A. -->|
|**Flash memory space**|<!-- How many flash memory space do you have in your device. Find this information with "SPI_FLASHINFO" command. -->|
|**Makefile configuration**|<!-- Tell which configuration/settings you tuned in Makefile. If it is not applicable or if you did not touch Makefile, put N/A. -->|

## Bug description
### Expected function and references
<!-- Please describe how things are supposed to work from your perspective. Do not hesitate to give URL or paths to this repository or Wiki pages to reference what you are trying to achieve. -->
### Bug
<!-- Please describe the bug you identified, i.e. in what ways it did not function as expected. -->
<!-- Give as much details as possible. Include commands, terminal output, and/or code lines where relevant, between back-quotes `like this` for a simple line, or between back-quotes paragraphs for multiple lines ``` LIKE THIS ```. -->
### Steps to Reproduce
<!-- Provide a link to a live example, images if relevant (GUI for instance), and/or an unambiguous set of actions to reproduce this bug. Include commands, terminal output, and/or code lines to reproduce, between back-quotes `like this` if relevant. -->
1.
2. ...

## Resolution paths
### Ideas
<!-- Describe any idea you have to fix the issue, if any. Put N/A if you do not have any. -->
### Possible Implementation
<!-- If you have any suggested implementation to fix the issue, including code, put it here between back-quotes paragraphs ``` LIKE THIS ```. Put N/A if you do not have any. -->

<!-- Thanks in advance for submitting a complete bug report -->

