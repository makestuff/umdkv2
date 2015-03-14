#!/bin/sh
#
# Copyright (C) 2011 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# sudo apt-get install libmotif-dev
# sudo apt-get install libx11-dev
# sudo apt-get install libxext-dev
# sudo apt-get install libxp-dev
# sudo apt-get install libxmu-dev
cd ${HOME}/src
if [ ! -e ddd-3.3.12.tar.gz ]; then
    wget http://ftp.gnu.org/gnu/ddd/ddd-3.3.12.tar.gz
fi
rm -rf ddd-3.3.12
tar zxf ddd-3.3.12.tar.gz
cd ddd-3.3.12
patch ddd/strclass.C <<EOF
--- ddd-3.3.12-new/ddd/strclass.C	2011-06-25 19:58:01.344386001 +0100
+++ ddd-3.3.12-old/ddd/strclass.C	2009-02-11 17:25:06.000000000 +0000
@@ -41,2 +41,3 @@
 #include <stdlib.h>
+#include <stdio.h>
 
EOF
patch ddd/GDBAgent.C <<EOF
--- ddd-3.3.12-new/ddd/GDBAgent.C	2011-06-25 20:03:55.464386002 +0100
+++ ddd-3.3.12-old/ddd/GDBAgent.C	2009-02-11 17:25:06.000000000 +0000
@@ -3202,3 +3202,3 @@
 	normalize_address(end_);
+	cmd += ',';
-	cmd += ' ';
 	cmd += end_;
EOF
patch ddd/SourceView.C <<EOF
--- ddd-3.3.12-old/ddd/SourceView.C	2009-02-11 17:25:06.000000000 +0000
+++ ddd-3.3.12-new/ddd/SourceView.C	2014-04-10 21:21:18.711630585 +0100
@@ -3978,3 +3978,5 @@
     string file_name = current_file_name;
-
+	if ( position.contains("0x", 0) ) {
+		return;
+	}
     if (position.contains(':'))
EOF
./configure --prefix=${HOME}/x-tools/m68k-megadrive-elf
make
make install
cd ..
rm -rf ddd-3.3.12
