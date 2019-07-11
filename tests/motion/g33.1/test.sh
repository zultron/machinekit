#!/bin/bash

rm -f ./samples
linuxcnc -r test.ini
exit $?
