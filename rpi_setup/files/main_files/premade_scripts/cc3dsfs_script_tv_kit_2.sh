#!/bin/sh

${PREMADE_DIR}/${PROGRAM_NAME}_prepare_script.sh

${MAIN_KIOSK_PROGRAM} ${TOUCH_COMMAND} --mono_app --pi_power 3 --pi_select 6 --pi_enter 9 --pi_menu 19

${PREMADE_DIR}/${PROGRAM_NAME}_post_script.sh $?
