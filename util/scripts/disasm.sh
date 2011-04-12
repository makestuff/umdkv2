#!/bin/bash

if [ "$#" != 2 ]; then
	echo "Synopsis: $0 <elfFile> <address>"
	exit 1
fi

/home/chris/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objdump -m68000 -D --start-address=$2 --stop-address=$(($2 + 256)) $1 | tail -n +8 | head -18 | perl -ane 's/\s<_binary.*?>//g;print;'

#/home/chris/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-objdump -m68000 --prefix-addresses --show-raw-insn --start-address=$2 --stop-address=$(($2 + 96)) -d -D $1 | perl -ane 's/\s<.*?>//g;print;' | grep --color=never -E '^00' | head -16
# | perl -ane 's/\s<.*?>//g;print;' | perl -ane 's/%([^,]*?)@\((.*?)\)/$2($1)/g;print;' | perl -ane 's/%([^,]*?)@/($1)/g;print;' | sed 's/,/, /g' | sed s/%//g | perl -ane 'if(/^00(......).(....).(....).(....)\s+(.*?)\s(.*?)$/){printf "00%s  %s %s %s      %-8s %s\n", uc($1), uc($2), uc($3), uc($4), $5, $6;}' | head -16
