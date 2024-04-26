
VOL := d3xx-osx.${VER}
SRC_FOLDER := /Volumes/${VOL}
TAR := ${VOL}.dmg

hdiutil attach ${1}
mkdir -p ${3}
cp ${2}/${4} ${3}
cp ${2}/*.h ${3}
hdiutil detach ${3}
