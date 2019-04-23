#!/bin/bash
#
# Set up .var file with suitable defaults
#
cp offsets-stock.var.template offsets-stock.var
cp offsets-extended.var.template offsets-extended.var

rs274 -g -v offsets.var offsets-stock.ngc
rs274 -g -v offsets-extended.var offsets-extended.ngc

