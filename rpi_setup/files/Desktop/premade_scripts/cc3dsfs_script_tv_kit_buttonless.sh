#!/bin/sh

${HOME}/Desktop/premade_scripts/cc3dsfs_prepare_script.sh ; ${HOME}/Desktop/cc3dsfs ${TOUCH_COMMAND} --mono_app
${HOME}/Desktop/premade_scripts/cc3dsfs_post_script.sh $?
