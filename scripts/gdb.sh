#!/bin/sh
rm -f /tmp/gdb.cmd
echo "set remotelogfile /tmp/gdb.log" >> /tmp/gdb.cmd
if [ $# = 1 ]; then
  # Code is already loaded and running; connect and interrupt it now:
  echo "set remote interrupt-on-connect yes" >> /tmp/gdb.cmd
  echo "target remote localhost:$1" >> /tmp/gdb.cmd
  echo "set radix 16" >> /tmp/gdb.cmd
  gdb -x /tmp/gdb.cmd
elif [ $# = 2 ]; then
  # Code has not been loaded; have GDB load it for us:
  echo "set remote interrupt-on-connect yes" >> /tmp/gdb.cmd
  echo "target remote localhost:$1" >> /tmp/gdb.cmd
  echo "load" >> /tmp/gdb.cmd
  echo "tbreak main" >> /tmp/gdb.cmd
  echo "set radix 16" >> /tmp/gdb.cmd
  echo "set \$pc=0x4000dc" >> /tmp/gdb.cmd
  echo "cont" >> /tmp/gdb.cmd
  gdb -x /tmp/gdb.cmd $2
else
  echo "Synopsis: gdb.sh <port> [<elfFile>]"
fi
