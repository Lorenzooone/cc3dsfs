#!/bin/sh

${HOME}/Desktop/premade_scripts/cc3dsfs_prepare_script.sh ; ${HOME}/Desktop/cc3dsfs ${TOUCH_COMMAND} --mono_app --pi_enter 27 --pi_select 9 --pi_menu 19
${HOME}/Desktop/premade_scripts/cc3dsfs_post_script.sh $?
