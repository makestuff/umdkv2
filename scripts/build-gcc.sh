#!/bin/bash

# This builds a toolchain in /usr/local/x-tools/m68k-unknown-elf/
#sudo apt-get install bison
#sudo apt-get install flex
#sudo apt-get install texinfo
#sudo apt-get install automake
#sudo apt-get install libtool
#sudo apt-get install zlib1g-dev
export VERSION=1.11.2
cd $HOME
rm -rf crosstool-ng
mkdir crosstool-ng
cd crosstool-ng
wget http://ymorin.is-a-geek.org/download/crosstool-ng/crosstool-ng-${VERSION}.tar.bz2
bunzip2 -c crosstool-ng-${VERSION}.tar.bz2 | tar xf -
cd crosstool-ng-${VERSION}/
./configure --prefix=$HOME/crosstool-ng/ct-ng
make
make install
cd ..
mkdir dev
cd dev
cp ../ct-ng/lib/ct-ng-${VERSION}/samples/m68k-unknown-elf/crosstool.config .config
HOME=/usr/local sudo $HOME/crosstool-ng/ct-ng/bin/ct-ng build
