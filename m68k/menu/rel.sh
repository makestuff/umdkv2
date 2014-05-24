#!/bin/sh
export SGDK_SRC=../sgdk095/src
make -C ${SGDK_SRC} clean
make -C ${SGDK_SRC}
make clean
make ORIGIN=0x420000
