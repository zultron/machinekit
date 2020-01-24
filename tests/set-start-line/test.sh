#!/bin/bash

cp sim.tbl.template sim.tmp.tbl
linuxcnc -r test.ini modal_state_test.ngc 21 --prep-type 1

