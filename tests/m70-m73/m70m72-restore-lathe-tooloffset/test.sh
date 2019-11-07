#!/bin/bash
cp tool.tbl.original tool.tmp.tbl
rs274 -t tool.tmp.tbl -i test.ini -g test.ngc | awk '{$1=""; print}'
exit ${PIPESTATUS[0]}
