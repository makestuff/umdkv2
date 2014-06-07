#!/bin/sh
export SGDK_SRC=../sgdk095/src
make -C ${SGDK_SRC} clean
make -C ${SGDK_SRC}
make clean

# For the actual on-flash release
make ORIGIN=0x420000

# For an optimised build that can be USB-loaded for testing
#make
