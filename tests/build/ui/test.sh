#!/bin/sh
set -x
CPPFLAGS=$(pkg-config --silence-errors --cflags libtirpc)
LDFLAGS=$(pkg-config --silence-errors --libs libtirpc)
g++ ${CPPFLAGS} -std=c++11 -I $EMC2_HOME/include \
    nml-position-logger.cc \
    -L $EMC2_HOME/lib -lnml -llinuxcnc ${LDFLAGS} \
    -o /dev/null
