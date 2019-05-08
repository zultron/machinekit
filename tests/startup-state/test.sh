#!/bin/bash

rm -f sim.var
cp sim.var.baseline sim.var
linuxcnc -r test.ini

