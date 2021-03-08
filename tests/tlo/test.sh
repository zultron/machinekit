#!/bin/bash

cp -f simpockets.tbl.original simpockets.tmp.tbl

linuxcnc -r g43-test.ini
exit $?

