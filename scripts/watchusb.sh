#!/bin/bash
echo Watching USB bus for devices being added and removed...
lsusb | awk '{print $6}' > new.lst
while [ 1 ]; do
	mv new.lst old.lst
	lsusb | awk '{print $6}' > new.lst
	diff old.lst new.lst | perl -ane 'if(m/^>/){print "Added: $F[1]\n";}elsif(m/^</){print "Removed: $F[1]\n";}'
	sleep 0.1
done
