#!/bin/sh

#sudo apt-get install cvs
#sudo apt-get install lzma
#sudo apt-get install bison
#sudo apt-get install flex
#sudo apt-get install texinfo
#sudo apt-get install automake
#sudo apt-get install libtool
#sudo apt-get install zlib1g-dev
export VERSION=1.19.0
cd $HOME
rm -rf crosstool-ng
if [ -e x-tools ]; then
  chmod +w -R x-tools
  rm -rf x-tools
fi
mkdir -p ${HOME}/src
mkdir crosstool-ng
cd crosstool-ng
wget http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-${VERSION}.tar.bz2
bunzip2 -c crosstool-ng-${VERSION}.tar.bz2 | tar xf -
cd crosstool-ng-${VERSION}/
./configure --prefix=/usr/local
make
sudo make install
cd ..
mkdir build
cd build
ct-ng m68k-unknown-elf
patch .config <<EOF
--- config.old	2014-04-02 12:40:43.552014000 +0100
+++ config.new	2014-04-02 12:44:53.896750988 +0100
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
ct-ng build
echo
echo
echo 'Now you can do:'
echo '  export PATH=${PATH}:${HOME}/x-tools/m68k-unknown-elf/bin'
