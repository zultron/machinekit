#!/bin/bash

# load an emc.var file into redis
# server must be running
# usage: cat /home/mah/emc2-tc/configs/smithy/emc.var|sh emcvarload.sh

awk '{ printf "HSET param %s %s\n", $1, $2 }'  |redis-cli 


