#!/usr/bin/env python3

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
from testing.linuxcnc_test_support import wait_for_linuxcnc_startup


c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

# Wait for LinuxCNC to initialize itself so the Status buffer stabilizes.
wait_for_linuxcnc_startup(s)

c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.home(-1)
c.wait_complete()

c.mode(linuxcnc.MODE_MDI)

feed_rate = 234.5
c.mdi('g94 ; normal "units per minute" feed mode')
c.mdi('f%.1f' % feed_rate)
c.wait_complete()
c.abort()
c.wait_complete()
s.poll()
print ("feed rate:", s.settings[1])
assert(math.fabs(s.settings[1] - feed_rate) < 0.0000001)

feed_rate = 345.6
c.mdi('g95 ; "units per revolution" feed mode')
c.mdi('f%.1f' % feed_rate)
c.wait_complete()
c.abort()
c.wait_complete()
s.poll()
print ("feed rate:", s.settings[1])
assert(math.fabs(s.settings[1] - feed_rate) < 0.0000001)

sys.exit(0)

