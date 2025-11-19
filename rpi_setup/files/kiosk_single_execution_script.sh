#!/bin/sh

KIOSK_LAUNCHER=startx

TARGET_EXIST_FILE=/tmp/executed_once

if [ ! -e ${TARGET_EXIST_FILE} ]; then
	touch ${TARGET_EXIST_FILE}
	${KIOSK_LAUNCHER} ${HOME}/command_to_execute_once.sh
fi
