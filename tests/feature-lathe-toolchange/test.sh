#!/bin/bash -e
# reset the tool table to a known starting configuration
rm -f tool.tbl
cp tool.tbl.original tool.tbl

# Test tool offsets after `M6 T[#<toolno>]` and
# `M61 Q[#<toolno>] G43 H[#<toolno>]`
rs274 -t tool.tbl -i test.ini -g test-good.ngc 2>&1

# Test that `G43` with no H-word is an error
! rs274 -i test.ini -g test-bad.ngc 2>&1 1>/dev/null
