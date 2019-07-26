#!/bin/bash

rm -f sim.var
cp sim.var.template sim.var
linuxcnc -r test.ini | grep ^CHECK
