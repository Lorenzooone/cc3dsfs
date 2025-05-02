#!/bin/sh

if [ "$(id -u)" -eq "0" ]; then
   echo "This script must be run as a regular user!";
   exit 1;
fi

PROGRAM_NAME=cc3dsfs
DEFAULT_SCRIPT_KIOSK_MODE=${PROGRAM_NAME}_script_tv_kit_2.sh

sudo apt update
sudo apt -y install xterm gpiod xserver-xorg xinit libxcursor1 x11-xserver-utils pipewire pipewire-alsa

sudo raspi-config nonint do_boot_behaviour B2

sudo systemctl disable avahi-daemon.service
sudo systemctl disable dphys-swapfile.service
sudo systemctl disable hciuart.service
sudo systemctl disable bluetooth.service
#sudo systemctl disable wpa_supplicant.service

# I know this isn't how you SHOULD do it, however I tried
# various other things (crontab @reboot and rc.local) and they didn't work...
echo "" >> ${HOME}/.bashrc
echo 'if [ -z "${EXECUTED_ONCE}" ]; then' >> ${HOME}/.bashrc
echo "  export EXECUTED_ONCE=1 ; startx ${HOME}/Desktop/${PROGRAM_NAME}_script.sh" >> ${HOME}/.bashrc
echo 'fi' >> ${HOME}/.bashrc

cp -Rf files/Desktop/ ${HOME}/
if [ $(getconf LONG_BIT) -eq 32 ]; then
  $(cd ${HOME}/Desktop ; rm -f ${PROGRAM_NAME} ; ln -s ${PROGRAM_NAME}_versions/${PROGRAM_NAME}_32 ${PROGRAM_NAME})
else
  $(cd ${HOME}/Desktop ; rm -f ${PROGRAM_NAME} ; ln -s ${PROGRAM_NAME}_versions/${PROGRAM_NAME}_64 ${PROGRAM_NAME})
fi
$(cd ${HOME}/Desktop ; rm -f ${PROGRAM_NAME}_script.sh ; ln -s premade_scripts/${DEFAULT_SCRIPT_KIOSK_MODE} ${PROGRAM_NAME}_script.sh)
chmod +x -R ${HOME}/Desktop/premade_scripts/
chmod +x -R ${HOME}/Desktop/${PROGRAM_NAME}_versions/
chmod +x ${HOME}/Desktop/${PROGRAM_NAME}
chmod +x ${HOME}/Desktop/${PROGRAM_NAME}_script.sh

if [ -e /boot/firmware/config.txt ]; then
  FIRMWARE=/firmware
else
  FIRMWARE=
fi

sudo cp -f files/config.txt /boot${FIRMWARE}/config.txt

sudo cp -Rf files/usr/ /
sudo cp -Rf files/etc/ /
sudo chmod +x /etc/rc.local

sleep 5

sudo reboot
