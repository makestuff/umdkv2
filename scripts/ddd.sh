#!/bin/bash

rm -f ddd.cmd
echo "set remotelogfile /var/tmp/gdb.log" >> ddd.cmd
if [ $# = 1 ]; then
	# Code is already loaded and running; connect and interrupt it now:
	echo "set remote interrupt-on-connect yes" >> ddd.cmd
	echo "target remote localhost:$1" >> ddd.cmd
	echo "set radix 16" >> ddd.cmd
	/usr/local/bin/ddd -geometry 827x660+600+830 --debugger /usr/local/bin/m68k-elf-gdb -x ddd.cmd &
elif [ $# = 2 ]; then
	# Code has not been loaded; have GDB load it for us:
	echo "target remote localhost:$1" >> ddd.cmd
	echo "load" >> ddd.cmd
	echo "tbreak main" >> ddd.cmd
	echo "set radix 16" >> ddd.cmd
	echo "cont" >> ddd.cmd
	/usr/local/bin/ddd -geometry 827x660+600+830 --debugger /usr/local/bin/m68k-elf-gdb -x ddd.cmd $2 &
else
	echo "Synopsis: ddd.sh <port> [<elfFile>]"
fi
