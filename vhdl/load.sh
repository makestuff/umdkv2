#!/bin/bash

if [ "$#" != 1 ]; then
	echo "Synopsis: $0 <m|s>"
	exit 1
fi
if [ "$1" = "m" ]; then
	export ROM=../m68k/menu/menu.bin
elif [ "$1" = "s" ]; then
	export ROM=../../sonic1.bin
else
	echo "Synopsis: $0 <m|s>"
	exit 2
fi
#cd ../vhdl
#sudo xilprg -c nero:04B4:8613 -b TopLevel.bit
cd ../mdsh
sudo ./mdsh <<EOF
load $ROM
quit
EOF
