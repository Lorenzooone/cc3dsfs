#!/bin/sh

wpctl set-volume @DEFAULT_AUDIO_SINK@ 1.0 ; xset s noblank ; xset s off ; xset -dpms ; xrandr --output HDMI-1 --auto ; sleep 5
