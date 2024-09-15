#!/bin/bash

BASE_AMD_STR="deb [arch=amd64,i386] http://archive.ubuntu.com/ubuntu/ "
BASE_SECURITY_AMD_STR="deb [arch=amd64,i386] http://security.ubuntu.com/ubuntu/ "
BASE_ARM_STR="deb [arch=arm64,armhf] http://ports.ubuntu.com/ "
NO_SPECIAL_STR=" main multiverse universe"
SECURITY_STR="-security main multiverse universe"
BACKPORTS_STR="-backports main multiverse universe"
UPDATES_STR="-updates main multiverse universe"

LOCATION_BASE="/etc/apt/sources.list.d"
LOCATION_AMD="${LOCATION_BASE}/x86_compilers.list"
LOCATION_ARM="${LOCATION_BASE}/arm_compilers.list"

LIST_ARCHITECTURES=("amd64" "i386" "arm64" "armhf")
LIST_PACKAGES_BASE=("libxinerama-dev" "libxi-dev" "libxss-dev" "libxxf86vm-dev" "libxkbfile-dev" "libxv-dev" "libx11-dev" "libxrandr-dev" "libxcursor-dev" "libudev-dev" "libflac-dev" "libvorbis-dev" "libgl1-mesa-dev" "libegl1-mesa-dev" "libdrm-dev" "libgbm-dev" "libfreetype-dev")
LIST_PACKAGES_NORMAL=("g++-multilib-i686-linux-gnu" "g++-aarch64-linux-gnu" "g++-arm-linux-gnueabihf" "g++:amd64" "g++" "git" "xorg-dev")
LIST_PACKAGES_SPECIAL=("libgpiod-dev")

echo "${BASE_AMD_STR}$(lsb_release -sc)${NO_SPECIAL_STR}" > ${LOCATION_AMD}
echo "${BASE_SECURITY_AMD_STR}$(lsb_release -sc)${SECURITY_STR}" >> ${LOCATION_AMD}
echo "${BASE_AMD_STR}$(lsb_release -sc)${BACKPORTS_STR}" >> ${LOCATION_AMD}
echo "${BASE_AMD_STR}$(lsb_release -sc)${UPDATES_STR}" >> ${LOCATION_AMD}

echo "${BASE_ARM_STR}$(lsb_release -sc)${NO_SPECIAL_STR}" > ${LOCATION_ARM}
echo "${BASE_ARM_STR}$(lsb_release -sc)${SECURITY_STR}" >> ${LOCATION_ARM}
echo "${BASE_ARM_STR}$(lsb_release -sc)${BACKPORTS_STR}" >> ${LOCATION_ARM}
echo "${BASE_ARM_STR}$(lsb_release -sc)${UPDATES_STR}" >> ${LOCATION_ARM}

#for a in ${LIST_ARCHITECTURES[@]};
#do
#	dpkg --add-architecture ${a}
#done

dpkg --add-architecture $1
apt update

#for a in ${LIST_ARCHITECTURES[@]};
#do
#	PACKAGES=""
#	for p in ${LIST_PACKAGES_BASE[@]};
#	do
#		PACKAGES="${PACKAGES} ${p}:${a}"
#	done
#	apt install -y ${PACKAGES}
#done

PACKAGES=""
for p in ${LIST_PACKAGES_SPECIAL[@]};
do
	PACKAGES="${PACKAGES} ${p}:$1"
done
apt install -y ${PACKAGES}

PACKAGES=""
for p in ${LIST_PACKAGES_BASE[@]};
do
	PACKAGES="${PACKAGES} ${p}:$1"
done
apt install -y ${PACKAGES}

PACKAGES=""
for p in ${LIST_PACKAGES_NORMAL[@]};
do
	PACKAGES="${PACKAGES} ${p}"
done
apt install -y ${PACKAGES}

