#!/bin/bash
# simple script to install and refreash the udev rules.
# 
sudo cp -rf driver/98-ChameleonMini.rules /etc/udev/rules.d/98-ChameleonMini.rules
sudo udevadm control --reload-rules

if [ -f "/etc/arch-release" ]; then #If user is running ArchLinux
	sudo usermod -aG uucp $(USER) #Use specific command and group
else
	sudo adduser $(USER) dialout
fi