hdiutil attach ${1}
cp ${2}/release/build/${4} ${3}
cp ${2}/release/*.h ${3}
hdiutil detach ${2} || true
