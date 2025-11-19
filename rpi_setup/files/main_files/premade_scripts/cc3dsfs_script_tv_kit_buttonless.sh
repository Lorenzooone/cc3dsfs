#!/bin/sh

${PREMADE_DIR}/${PROGRAM_NAME}_prepare_script.sh

${MAIN_KIOSK_PROGRAM} ${TOUCH_COMMAND} --mono_app

${PREMADE_DIR}/${PROGRAM_NAME}_post_script.sh $?
