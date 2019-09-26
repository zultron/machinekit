#!/bin/bash

linuxcnc -r test.ini | grep ^CHECK
