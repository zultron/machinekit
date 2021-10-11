#!/bin/bash

rm -f tool.tmp.tbl
cp tool.tbl.original tool.tmp.tbl

linuxcnc -r test.ini
exit $?

