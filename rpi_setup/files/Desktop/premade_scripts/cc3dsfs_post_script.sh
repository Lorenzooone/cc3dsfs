#!/bin/sh

error=$1

if [ -e ${TO_TOUCH_FILE} ]; then
	if [ $error -ne 0 ]; then
		rm -f ${TO_TOUCH_FILE}
		echo "Shutting down!"
		shutdown now
	fi
	rm -f ${TO_TOUCH_FILE}
fi
