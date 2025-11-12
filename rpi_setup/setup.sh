#!/bin/sh

if [ "$(id -u)" -eq "0" ]; then
   echo "This script must be run as a regular user!";
   exit 1;
fi

PROGRAM_NAME=cc3dsfs
BASE_SOURCE_DIR="."
BASE_TARGET_DIR=${HOME}


sudo apt update
sudo apt -y install xterm xserver-xorg xinit libxcursor1 x11-xserver-utils pipewire pipewire-alsa libharfbuzz-icu0 lsb-release
if [ $? -ne 0 ]; then
    echo "Error while installing required packages! Exiting early!"
    exit 1
fi

codename=$(lsb_release -sc)
echo "Types: deb deb-src
URIs: http://deb.debian.org/debian
Suites: ${codename}-backports
Components: main
Enabled: yes
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg" > backports
sudo mv backports /etc/apt/sources.list.d/debian-backports.sources

sudo apt update
sudo apt -y install libgpiod3 || sudo apt -y install -t ${codename}-backports libgpiod3
if [ $? -ne 0 ]; then
    echo "Error while installing required package libgpiod3! Exiting early!"
    exit 1
fi

sudo raspi-config nonint do_boot_behaviour B2

sudo systemctl disable avahi-daemon.service
sudo systemctl disable dphys-swapfile.service
sudo systemctl disable hciuart.service
sudo systemctl disable bluetooth.service
#sudo systemctl disable wpa_supplicant.service

# I know this isn't how you SHOULD do it, however I tried
# various other things (crontab @reboot and rc.local) and they didn't work...
echo "" >> ${BASE_TARGET_DIR}/.bashrc
echo 'if [ -z "${EXECUTED_ONCE}" ]; then' >> ${BASE_TARGET_DIR}/.bashrc
echo "  export EXECUTED_ONCE=1 ; startx ${BASE_TARGET_DIR}/Desktop/${PROGRAM_NAME}_script.sh" >> ${BASE_TARGET_DIR}/.bashrc
echo 'fi' >> ${BASE_TARGET_DIR}/.bashrc

rm -f ${BASE_TARGET_DIR}/Desktop/${PROGRAM_NAME}_script.sh
cp -Rf ${BASE_SOURCE_DIR}/files/Desktop/ ${BASE_TARGET_DIR}/
if [ $(getconf LONG_BIT) -eq 32 ]; then
  $(cd ${BASE_TARGET_DIR}/Desktop ; rm -f ${PROGRAM_NAME} ; ln -s ${PROGRAM_NAME}_versions/${PROGRAM_NAME}_32 ${PROGRAM_NAME})
else
  $(cd ${BASE_TARGET_DIR}/Desktop ; rm -f ${PROGRAM_NAME} ; ln -s ${PROGRAM_NAME}_versions/${PROGRAM_NAME}_64 ${PROGRAM_NAME})
fi
chmod +x -R ${BASE_TARGET_DIR}/Desktop/premade_scripts/
chmod +x -R ${BASE_TARGET_DIR}/Desktop/${PROGRAM_NAME}_versions/
chmod +x ${BASE_TARGET_DIR}/Desktop/${PROGRAM_NAME}
chmod +x ${BASE_TARGET_DIR}/Desktop/${PROGRAM_NAME}_script.sh
chmod +x ${BASE_TARGET_DIR}/Desktop/update_to_unstable.sh
chmod +x ${BASE_TARGET_DIR}/Desktop/update_to_stable.sh

if [ -e /boot/firmware/config.txt ]; then
  FIRMWARE=/firmware
else
  FIRMWARE=
fi

sudo cp -f ${BASE_SOURCE_DIR}/files/config.txt /boot${FIRMWARE}/config.txt

sudo cp -Rf ${BASE_SOURCE_DIR}/files/usr/ /
sudo cp -Rf ${BASE_SOURCE_DIR}/files/etc/ /
sudo chmod +x /etc/rc.local
# Raspberry Pi OS comes with this now by default?! Why?!
sudo mkdir -p /etc/cloud/
sudo touch /etc/cloud/cloud-init.disabled

sleep 5

sudo reboot
