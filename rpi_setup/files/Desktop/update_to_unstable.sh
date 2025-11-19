#!/bin/sh

if [ -e /boot/firmware/config.txt ]; then
  FIRMWARE=/firmware
else
  FIRMWARE=
fi

PROGRAM_NAME=cc3dsfs
BASE_TARGET_DIR=/boot${FIRMWARE}/${PROGRAM_NAME}

${BASE_TARGET_DIR}/premade_scripts/update_script.sh https://github.com/Lorenzooone/cc3dsfs/releases/download/nightly-latest cc3dsfs_rpi_kiosk_setup.zip setup.sh
