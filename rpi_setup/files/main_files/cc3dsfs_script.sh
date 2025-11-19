#!/bin/sh

# You can change the contents of this file to specify a different script
# Do not change the path unless you know what you are doing
FILE_WITH_TARGET_SCRIPT=cc3dsfs_script_name.txt

# Do not change the following unless you know what you are doing
if [ -e /boot/firmware/config.txt ]; then
  FIRMWARE=/firmware
else
  FIRMWARE=
fi

export PROGRAM_NAME=cc3dsfs
export BASE_TARGET_DIR=/boot${FIRMWARE}/${PROGRAM_NAME}
export PREMADE_DIR=${BASE_TARGET_DIR}/premade_scripts
export CC3DSFS_CFG_DIR=${BASE_TARGET_DIR}/.config
export MAIN_KIOSK_PROGRAM=${HOME}/${PROGRAM_NAME}_link

mkdir -p ${CC3DSFS_CFG_DIR}

ENV_SCRIPT=${PROGRAM_NAME}_envs.sh

. ${PREMADE_DIR}/${ENV_SCRIPT}

TARGET_SCRIPT=$(cat ${BASE_TARGET_DIR}/${FILE_WITH_TARGET_SCRIPT})

${PREMADE_DIR}/${TARGET_SCRIPT}
