#!/bin/bash

cp sim.var.template param.tmp.var
linuxcnc -r test.ini | grep ^CHECK
