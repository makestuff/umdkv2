#!/bin/bash

if [ "$#" != 2 ]; then
	echo "Synopsis: $0 <binFile> <elfFile>"
	exit 1
fi

/home/chris/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objcopy -B 68000 -I binary -O elf32-m68k $1 $2
