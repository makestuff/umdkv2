#!/bin/bash

if [ "$#" != 2 ]; then
	echo "Synopsis: $0 <elfFile> <symbol>"
	exit 1
fi

printf "0x"
nm $1 | grep -E " $2$" | cut -d' ' -f1
