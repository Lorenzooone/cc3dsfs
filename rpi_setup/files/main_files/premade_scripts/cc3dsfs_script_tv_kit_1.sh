#!/bin/sh

# If you change this, you may want to do it in a copy, and point to it in
# cc3dsfs_script_name.txt, so it is not reverted when you update.

${PREMADE_DIR}/${PROGRAM_NAME}_prepare_script.sh

${MAIN_KIOSK_PROGRAM} ${EXTRA_COMMANDS} ${TOUCH_COMMAND} --mono_app --pi_enter 27 --pi_select 9 --pi_menu 19

${PREMADE_DIR}/${PROGRAM_NAME}_post_script.sh $?
