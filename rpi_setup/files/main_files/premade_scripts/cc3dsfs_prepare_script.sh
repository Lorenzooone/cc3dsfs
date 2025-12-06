#!/bin/sh

# If you change this, you may want to do it in a copy, and point to it in other
# scripts, so it is not reverted when you update.

# Examples of forcing a specific output mode. By default, it is automatic.
# When using a capture card, changing this default may be needed...
#OUTPUT_MODE_INFO="--mode 1920x1080 --rate 60"
#OUTPUT_MODE_INFO="--mode 1920x1080 --rate 120"
#OUTPUT_MODE_INFO="--mode 3840x2160 --rate 60"
OUTPUT_MODE_INFO="--auto"

wpctl set-volume @DEFAULT_AUDIO_SINK@ 1.0 ; xset s noblank ; xset s off ; xset -dpms ; xrandr --output HDMI-1 ${OUTPUT_MODE_INFO} ; rm -f ${TO_TOUCH_FILE} ; sleep 5
