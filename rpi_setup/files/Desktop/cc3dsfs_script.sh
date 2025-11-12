#!/bin/sh

TARGET_SCRIPT=cc3dsfs_script_tv_kit_2.sh

BASE_DIR=${HOME}/Desktop/premade_scripts

ENV_SCRIPT=cc3dsfs_envs.sh

. ${BASE_DIR}/${ENV_SCRIPT}
${BASE_DIR}/${TARGET_SCRIPT}
