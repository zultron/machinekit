#!/bin/sh
set -xe 
HEADERS=$(readlink -f ../../../include)
if test -n "$HAL_PKG"; then
    CFLAGS="$(pkg-config --cflags ${HAL_PKG})"
fi
for i in $HEADERS/*.h; do
    case $i in
    */rtapi_app.h) continue ;;
    esac
    gcc -DULAPI $CFLAGS -I$HEADERS -E -x c $i > /dev/null
done
for i in $HEADERS/*.h $HEADERS/*.hh; do
    case $i in
    */rtapi_app.h) continue ;;
    */interp_internal.hh) continue ;;
    esac
    g++ -DULAPI $CFLAGS -I$HEADERS -E -x c++ $i > /dev/null
done
