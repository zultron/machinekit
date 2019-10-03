#!/bin/bash

cp sim.var.baseline param.tmp.var
linuxcnc -r test.ini

