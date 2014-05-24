#!/bin/sh
export SGDK_SRC=../sgdk095/src
make -C ${SGDK_SRC} clean
make -C ${SGDK_SRC} OPTS=-g
make clean
make OPTS=-g
