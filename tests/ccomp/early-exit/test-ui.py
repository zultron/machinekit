#!/usr/bin/env python

import linuxcnc
import hal

import math
import time
import sys
import subprocess
import os
import signal
import glob
import re
from linuxcnc_test_support import wait_for_linuxcnc_startup


c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

# Wait for LinuxCNC to initialize itself so the Status buffer stabilizes.
wait_for_linuxcnc_startup(s)

#time.sleep(9999)

c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.home(-1)
c.wait_complete()

c.mode(linuxcnc.MODE_MDI)
c.mdi('g0 x0.1 y0.2 z0.3')
c.mdi('g41.1 d0.1')
c.mdi('g40')
c.mdi('g0 x0.9  ; surprise motion on Y and Z, to 0!!')
c.wait_complete()

s.poll()
print "position:", s.position
assert(math.fabs(s.position[0] - 0.9) < 0.0000001)
assert(math.fabs(s.position[1] - 0.2) < 0.0000001)
assert(math.fabs(s.position[2] - 0.3) < 0.0000001)

sys.exit(0)

