#!/bin/sh
set -xe

TOPDIR=`readlink -f ../../..`
INCLUDE=$TOPDIR/include
LIB=$TOPDIR/lib
eval `grep ^PYTHON_CPPFLAGS= $TOPDIR/src/Makefile.inc`

g++ -std=c++11 -o use-rs274 use-rs274.cc \
    -Wall -Wextra -Wno-return-type -Wno-unused-parameter \
    -I $INCLUDE $PYTHON_CPPFLAGS -L $LIB -Wl,-rpath,$LIB -lrs274
./use-rs274
