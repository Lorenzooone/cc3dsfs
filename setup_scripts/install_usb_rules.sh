#!/bin/sh

if [ "$(id -u)" -ne "0" ]; then
	echo "This script shoud be run as root!";
	exit 1;
fi

cd -P -- "$(dirname -- "$0")"

# Default...
RULES_DIR=.

if [ -d "usb_rules" ]; then
	# Handle releases
	RULES_DIR=usb_rules
elif [ -d "../usb_rules" ]; then
	# Handle people self-compiling
	RULES_DIR=../usb_rules
fi

REAL_PATH_RULES=$(realpath $RULES_DIR)
echo "Looking inside $REAL_PATH_RULES for *.rules files..."

cp -vf $RULES_DIR/*.rules /etc/udev/rules.d/

udevadm control --reload-rules
udevadm trigger
