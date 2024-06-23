hdiutil attach ${1}
cp ${2}/${4} ${3}
cp ${2}/*.h ${3}
hdiutil detach ${2} || true
