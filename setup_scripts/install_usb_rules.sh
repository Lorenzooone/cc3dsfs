cp *.rules /etc/udev/rules.d/
udevadm control --reload-rules
udevadm trigger
