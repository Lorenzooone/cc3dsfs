#!/bin/sh
# Example script for 4 3DSs connected at the same time
# Both screens, placed in an horizontal grid
# Scaled 1x

# Needed for Optimize 3DS CCs to reconnect
SLEEP_TIME=10

# Set to 1 to enable configuration mode with only one window active.
# Set to 0 to enable all windows.
INITIAL_CONFIGURATION_MODE=0

${PREMADE_DIR}/${PROGRAM_NAME}_prepare_script.sh

if [ "$INITIAL_CONFIGURATION_MODE" -eq "1" ]; then

# Rightmost
${MAIN_KIOSK_PROGRAM} --enabled_both 1 --pos_x_both 1200 --pos_y_both 0 --profile 5 --auto_connect &
sleep ${SLEEP_TIME}

# Middle-Right
${MAIN_KIOSK_PROGRAM} --enabled_both 1 --pos_x_both 800 --pos_y_both 0 --profile 5 --auto_connect &
sleep ${SLEEP_TIME}

# Middle-Left
${MAIN_KIOSK_PROGRAM} --enabled_both 1 --pos_x_both 400 --pos_y_both 0 --profile 5 --auto_connect &
sleep ${SLEEP_TIME}

fi

# Leftmost
${MAIN_KIOSK_PROGRAM} --enabled_both 1 --pos_x_both 0 --pos_y_both 0 --profile 5 --auto_connect

${PREMADE_DIR}/${PROGRAM_NAME}_post_script.sh $?
