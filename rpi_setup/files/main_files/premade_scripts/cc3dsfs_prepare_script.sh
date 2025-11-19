#!/bin/sh

wpctl set-volume @DEFAULT_AUDIO_SINK@ 1.0 ; xset s noblank ; xset s off ; xset -dpms ; xrandr --output HDMI-1 --auto ; rm -f ${TO_TOUCH_FILE} ; sleep 5
