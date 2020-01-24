#!/usr/bin/env python

import linuxcnc
import hal

import time
import sys
import os
import glob
import re

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
c.program_open('modal_state_test.ngc')
c.wait_complete()
time.sleep(.1)
c.mode(linuxcnc.MODE_AUTO)
c.auto(linuxcnc.AUTO_RUN, 0)
c.wait_complete(100)

# Run from the designated start line in the test program and log modal state
# KLUDGE need to know the specific line here...
c.program_open('modal_state_test.ngc')
c.wait_complete()
time.sleep(.1)
c.mode(linuxcnc.MODE_AUTO)
c.auto(linuxcnc.AUTO_RUN, 21, PREP_LINEAR_LEADIN, False)
c.wait_complete()
s.poll()

