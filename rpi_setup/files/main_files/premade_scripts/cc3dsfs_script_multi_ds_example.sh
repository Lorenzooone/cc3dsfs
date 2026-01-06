#!/bin/sh

# If you change this, you may want to do it in a copy, and point to it in
# cc3dsfs_script_name.txt, so it is not reverted when you update.

# Example script for 4 DSs connected at the same time
# Top screens, placed in a 4x4 grid, with space on all sides
# For 1920x1080
# Scaled 2x each

# Needed for Optimize 3DS CCs to reconnect
SLEEP_TIME=10

# Set to 1 to enable configuration mode with only one window active.
# Set to 0 to enable all windows.
INITIAL_CONFIGURATION_MODE=0

${PREMADE_DIR}/${PROGRAM_NAME}_prepare_script.sh

if [ "$INITIAL_CONFIGURATION_MODE" -eq "0" ]; then

# Bottom Right
${MAIN_KIOSK_PROGRAM} ${EXTRA_COMMANDS} --enabled_both 0 --enabled_top 1 --pos_x_top 1184 --pos_y_top 618 --scaling_top 2 --profile 6 --auto_connect &
sleep ${SLEEP_TIME}

# Bottom Left
${MAIN_KIOSK_PROGRAM} ${EXTRA_COMMANDS} --enabled_both 0 --enabled_top 1 --pos_x_top 224 --pos_y_top 618 --scaling_top 2 --profile 6 --auto_connect &
sleep ${SLEEP_TIME}

# Top Right
${MAIN_KIOSK_PROGRAM} ${EXTRA_COMMANDS} --enabled_both 0 --enabled_top 1 --pos_x_top 1184 --pos_y_top 78 --scaling_top 2 --profile 6 --auto_connect &
sleep ${SLEEP_TIME}

fi

# Top Left
${MAIN_KIOSK_PROGRAM} ${EXTRA_COMMANDS} --enabled_both 0 --enabled_top 1 --pos_x_top 224 --pos_y_top 78 --scaling_top 2 --profile 6 --auto_connect

${PREMADE_DIR}/${PROGRAM_NAME}_post_script.sh $?
