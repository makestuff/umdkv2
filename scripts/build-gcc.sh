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
#sudo apt-get install cvs
#sudo apt-get install lzma
#sudo apt-get install bison
#sudo apt-get install flex
#sudo apt-get install texinfo
#sudo apt-get install automake
#sudo apt-get install libtool
#sudo apt-get install zlib1g-dev
#sudo apt-get install gawk
#sudo apt-get install gperf
#sudo apt-get install libncurses5-dev
export VERSION=1.19.0
cd $HOME
rm -rf crosstool-ng
if [ -e x-tools ]; then
  chmod +w -R x-tools
  rm -rf x-tools
fi
mkdir -p src crosstool-ng x-tools
cd crosstool-ng
wget http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-${VERSION}.tar.bz2
bunzip2 -c crosstool-ng-${VERSION}.tar.bz2 | tar xf -
cd crosstool-ng-${VERSION}/
./configure --prefix=${HOME}/crosstool-ng
make
make install
cd ..
mkdir build
cd build
${HOME}/crosstool-ng/bin/ct-ng m68k-unknown-elf
patch .config <<EOF
--- config.old	2014-04-02 12:40:43.552014000 +0100
+++ config.new	2014-04-02 12:44:53.896750988 +0100
@@ -99 +99 @@
-CT_ARCH_CPU="cpu32"
+CT_ARCH_CPU="68000"
@@ -158 +158 @@
-CT_TARGET_VENDOR="unknown"
+CT_TARGET_VENDOR="megadrive"
@@ -201,2 +201,2 @@
-# CT_ARCH_BINFMT_ELF is not set
-CT_ARCH_BINFMT_FLAT=y
+CT_ARCH_BINFMT_ELF=y
+# CT_ARCH_BINFMT_FLAT is not set
@@ -234,8 +233,0 @@
-# elf2flt
-#
-CT_ELF2FLT_CVSHEAD=y
-# CT_ELF2FLT_CUSTOM is not set
-CT_ELF2FLT_VERSION="cvs"
-CT_ELF2FLT_EXTRA_CONFIG_ARRAY=""
-
-#
@@ -384 +376,6 @@
-# CT_COMP_TOOLS is not set
+CT_COMP_TOOLS=y
+# CT_COMP_TOOLS_make is not set
+# CT_COMP_TOOLS_m4 is not set
+# CT_COMP_TOOLS_autoconf is not set
+CT_COMP_TOOLS_automake=y
+# CT_COMP_TOOLS_libtool is not set
EOF
unset LD_LIBRARY_PATH
${HOME}/crosstool-ng/bin/ct-ng build
echo
echo
echo 'Now you can do:'
echo '  export PATH=${PATH}:${HOME}/x-tools/m68k-megadrive-elf/bin'
