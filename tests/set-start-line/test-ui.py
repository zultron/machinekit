#!/usr/bin/env python

import linuxcnc
import hal

import time
import sys
import os
import glob
import re
import argparse
import shutil

print sys.argv
parser = argparse.ArgumentParser()
parser.add_argument("-ini", help="ini specified", type=str)
parser.add_argument("ngc_file", help="G-code file to test for start line modal state", type=str)
parser.add_argument("start_line", help="start line number", type=int)
parser.add_argument("--prep-type", help="Prep type number (0 = none, 1 = linear lead, 2 = plunge lead)", type=int,choices=[0,1,2])
parser.add_argument("--keep-tool", help="Don't change to the tool needed by the start line (assume user loaded tool correctly)", action="store_true")
args = parser.parse_args()

ngc_filename = args.ngc_file+'.ngc'

PREP_NONE = 0
PREP_LINEAR_LEADIN = 1
PREP_PLUNGE_LEADIN = 2

c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

c.state(linuxcnc.STATE_ESTOP_RESET)
c.wait_complete()
c.state(linuxcnc.STATE_ON)

# Run the whole program from the beginning and save the modal state to a log file
c.mode(linuxcnc.MODE_AUTO)
c.wait_complete()
c.program_open(ngc_filename)
c.wait_complete()
time.sleep(.5)
c.mode(linuxcnc.MODE_AUTO)
c.auto(linuxcnc.AUTO_RUN, 0)
c.wait_complete(100)

# Run from the designated start line in the test program and log modal state
# KLUDGE need to know the specific line here...
c.program_open(ngc_filename)
c.wait_complete()
time.sleep(.5)
c.mode(linuxcnc.MODE_AUTO)
c.auto(linuxcnc.AUTO_RUN, args.start_line, args.prep_type, args.keep_tool)
c.wait_complete()
s.poll()

os.rename('run_from_start.log','{}_from_start.log'.format(args.ngc_file))
os.rename('run_from_line.log','{}_from_line.log'.format(args.ngc_file))
