#!/bin/sh

# If you change this, you may want to do it in a copy, and point to it in other
# scripts, so it is not reverted when you update.

error=$1

if [ -e ${TO_TOUCH_FILE} ]; then
	if [ $error -ne 0 ]; then
		rm -f ${TO_TOUCH_FILE}
		echo "Shutting down!"
		shutdown now
	fi
	rm -f ${TO_TOUCH_FILE}
fi
