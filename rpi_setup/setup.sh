#!/bin/sh

if [ "$(id -u)" -eq "0" ]; then
   echo "This script must be run as a regular user!";
   exit 1;
fi

if [ -e /boot/firmware/config.txt ]; then
  FIRMWARE=/firmware
else
  FIRMWARE=
fi

PROGRAM_NAME=cc3dsfs
BASE_SOURCE_DIR="."
BASE_TARGET_DIR=/boot${FIRMWARE}/${PROGRAM_NAME}

# cage provided as an alternative to startx.
# gldriver-test needed for Raspberry Pi 5... Weirdly enough?!
sudo apt update
sudo apt -y install xterm xserver-xorg xinit libxcursor1 x11-xserver-utils pipewire pipewire-alsa libharfbuzz-icu0 lsb-release cage gldriver-test
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

sudo mkdir -p ${BASE_TARGET_DIR}
# This won't work in /boot/firmware, but whatever...
# Keep it here if the folder ever changes again.
#sudo chown -R ${USER}:${USER} ${BASE_TARGET_DIR}

KIOSK_SINGLE_EXECUTION_SCRIPT=kiosk_single_execution_script.sh
cp -f ${BASE_SOURCE_DIR}/files/${KIOSK_SINGLE_EXECUTION_SCRIPT} ${HOME}/${KIOSK_SINGLE_EXECUTION_SCRIPT}
# Do this indirection so the name is not hardcoded...
echo "${BASE_TARGET_DIR}/${PROGRAM_NAME}_script.sh" > ${HOME}/command_to_execute_once.sh
chmod +x ${HOME}/${KIOSK_SINGLE_EXECUTION_SCRIPT}
chmod +x ${HOME}/command_to_execute_once.sh

# Backup bashrc, to be sure...
BACKUP_BASE_NAME=${HOME}/.bashrc_bak
BACKUP_AVAILABLE_NUMBER=0
BACKUP_FILE_NAME=$BACKUP_BASE_NAME

while [ -e "$BACKUP_FILE_NAME" ]; do
	BACKUP_FILE_NAME="${BACKUP_BASE_NAME}_${BACKUP_AVAILABLE_NUMBER}"
	BACKUP_AVAILABLE_NUMBER=$(($BACKUP_AVAILABLE_NUMBER+1))
done

cp ${HOME}/.bashrc ${BACKUP_FILE_NAME}

# I know this isn't how you SHOULD do it, however I tried
# various other things (crontab @reboot and rc.local) and they didn't work...
cp -f /etc/skel/.bashrc ${HOME}/.bashrc
echo "" >> ${HOME}/.bashrc
echo "${HOME}/${KIOSK_SINGLE_EXECUTION_SCRIPT}" >> ${HOME}/.bashrc

# Copy the real program...
sudo rm -f ${BASE_TARGET_DIR}/${PROGRAM_NAME}_script.sh
sudo cp -Rf ${BASE_SOURCE_DIR}/files/main_files/* ${BASE_TARGET_DIR}/
if [ $(getconf LONG_BIT) -eq 32 ]; then
  $(cd ${HOME} ; rm -f ${PROGRAM_NAME}_link ; ln -s ${BASE_TARGET_DIR}/${PROGRAM_NAME}_versions/${PROGRAM_NAME}_32 ${PROGRAM_NAME}_link)
else
  $(cd ${HOME} ; rm -f ${PROGRAM_NAME}_link ; ln -s ${BASE_TARGET_DIR}/${PROGRAM_NAME}_versions/${PROGRAM_NAME}_64 ${PROGRAM_NAME}_link)
fi
# This won't work in /boot/firmware, but whatever...
# Keep it here if the folder ever changes again.
#sudo chmod +x -R ${BASE_TARGET_DIR}/premade_scripts/
#sudo chmod +x -R ${BASE_TARGET_DIR}/${PROGRAM_NAME}_versions/
#sudo chmod +x ${BASE_TARGET_DIR}/${PROGRAM_NAME}
#sudo chmod +x ${BASE_TARGET_DIR}/${PROGRAM_NAME}_script.sh
sudo chmod +x ${HOME}/${PROGRAM_NAME}_link

cp -Rf ${BASE_SOURCE_DIR}/files/Desktop/ ${HOME}/
chmod +x ${HOME}/Desktop/update_to_unstable.sh
chmod +x ${HOME}/Desktop/update_to_stable.sh

# Add symbolic link if user boots wants to use the terminal
rm -f ${HOME}/update_to_unstable.sh
rm -f ${HOME}/update_to_stable.sh
ln -s ${HOME}/Desktop/update_to_unstable.sh ${HOME}/update_to_unstable.sh
ln -s ${HOME}/Desktop/update_to_stable.sh ${HOME}/update_to_stable.sh

# Add symbolic link if user boots Desktop
rm -f ${HOME}/Desktop/${PROGRAM_NAME}
rm -rf ${HOME}/Desktop/${PROGRAM_NAME}
ln -s ${BASE_TARGET_DIR} ${HOME}/Desktop/${PROGRAM_NAME}

# Add symbolic link if user wants to use the terminal
rm -f ${HOME}/${PROGRAM_NAME}
rm -rf ${HOME}/${PROGRAM_NAME}
ln -s ${BASE_TARGET_DIR} ${HOME}/${PROGRAM_NAME}

sudo cp -f ${BASE_SOURCE_DIR}/files/config.txt /boot${FIRMWARE}/config.txt

GROUP_NAME=boot_files
sudo groupadd -U ${USER} ${GROUP_NAME}

sudo cp -Rf ${BASE_SOURCE_DIR}/files/usr/ /
sudo cp -Rf ${BASE_SOURCE_DIR}/files/etc/ /
sudo chmod +x /etc/rc.local

# Add ability to increase priority... May not use it...
echo ${USER} nice hard -20 > /tmp/niceness
echo ${USER} nice soft -20 > /tmp/niceness
sudo mv /tmp/niceness /etc/security/limits.d/35-${PROGRAM_NAME}-nice-limits.conf

# Raspberry Pi OS comes with this now by default?! Why?!
sudo mkdir -p /etc/cloud/
sudo touch /etc/cloud/cloud-init.disabled

sleep 5

sudo reboot
