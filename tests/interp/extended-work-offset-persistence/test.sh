#!/bin/bash
#
# Set up .var file with suitable defaults
#
cp offsets-stock.var.template offsets-stock.var || exit 1
cp offsets-extended.var.template offsets-extended.var || exit 1

rs274 -g -v offsets.var offsets-stock.ngc || exit 1
rs274 -g -v offsets-extended.var offsets-extended.ngc || exit 1

