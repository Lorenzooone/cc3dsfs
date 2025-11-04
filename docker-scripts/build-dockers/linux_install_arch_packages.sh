#!/bin/bash

LIST_PACKAGES_BASE=("libxinerama-dev" "libxi-dev" "libxss-dev" "libxxf86vm-dev" "libxkbfile-dev" "libxv-dev" "libx11-dev" "libxrandr-dev" "libxcursor-dev" "libudev-dev" "libflac-dev" "libvorbis-dev" "libgl1-mesa-dev" "libegl1-mesa-dev" "libdrm-dev" "libgbm-dev" "libfreetype-dev" "libharfbuzz-dev")

dpkg --add-architecture $1
apt update

#PACKAGES=""
#for p in ${LIST_PACKAGES_SPECIAL[@]};
#do
#	PACKAGES="${PACKAGES} ${p}:$1"
#done
#apt install -y ${PACKAGES}

PACKAGES=""
for p in ${LIST_PACKAGES_BASE[@]};
do
	PACKAGES="${PACKAGES} ${p}:$1"
done
apt install -y ${PACKAGES}

