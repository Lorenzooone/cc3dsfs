#!/bin/sh

if [ "$(id -u)" -eq "0" ]; then
   echo "This script must be run as a regular user!";
   exit 1;
fi

DEFAULT_SCRIPT_KIOSK_MODE=cc3dsfs_script_tv_kit_2.sh

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
echo "  export EXECUTED_ONCE=1 ; startx ${HOME}/Desktop/cc3dsfs_script.sh" >> ${HOME}/.bashrc
echo 'fi' >> ${HOME}/.bashrc

cp -Rf files/Desktop/ ${HOME}/
if [ $(getconf LONG_BIT) -eq 32 ]; then
  $(cd ${HOME}/Desktop ; ln -s cc3dsfs_versions/cc3dsfs_32 cc3dsfs)
else
  $(cd ${HOME}/Desktop ; ln -s cc3dsfs_versions/cc3dsfs_64 cc3dsfs)
fi
$(cd ${HOME}/Desktop ; ln -s premade_scripts/${DEFAULT_SCRIPT_KIOSK_MODE} cc3dsfs_script.sh)
chmod +x -R ${HOME}/Desktop/premade_scripts/
chmod +x -R ${HOME}/Desktop/cc3dsfs_versions/
chmod +x ${HOME}/Desktop/cc3dsfs
chmod +x ${HOME}/Desktop/cc3dsfs_script.sh

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
