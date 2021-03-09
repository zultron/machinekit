#!/bin/bash

cp -f ../simpockets.tbl.original simpockets.tmp.tbl

linuxcnc -r m61-test.ini
exit $?

