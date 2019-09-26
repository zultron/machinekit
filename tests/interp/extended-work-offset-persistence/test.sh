#!/bin/bash
#
# Set up .var file with suitable defaults
#
cp offsets-stock.var.template offsets-stock.tmp.var || exit 1
cp offsets-extended.var.template offsets-extended.tmp.var || exit 1

rs274 -g -v offsets.tmp.var offsets-stock.ngc || exit 1
rs274 -g -v offsets-extended.tmp.var offsets-extended.ngc || exit 1

