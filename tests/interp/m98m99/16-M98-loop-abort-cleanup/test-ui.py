#!/usr/bin/env python

import linuxcnc
import hal

import time
import sys
from testing.linuxcnc_test_support import *


# connect to LinuxCNC
c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

wait_for_linuxcnc_startup(s)

# Come out of E-stop, turn the machine on, home, and switch to Auto mode.
c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.mode(linuxcnc.MODE_AUTO)

repetitions = 10
# run the .ngc test file which will fail due to the ABORT statement in the subroutine
for k in range(repetitions):
    print("Running test program, iteration {}".format(k), file=sys.stderr)
    c.program_open('test.ngc')
    c.wait_complete()
    c.auto(linuxcnc.AUTO_RUN, 1)
    c.wait_complete(100)
    s.poll()
    float_assert(s.position[0], 0, comment="expect no X movement due to early abort")
    float_assert(s.position[1], 0, comment="expect no Y movement due to early abort")
    float_assert(s.position[2], 0.01, comment = "expect Z position to be unchanged after initial move")
    # Expect an error every time no matter what
    while True:
        res = e.poll()
        if res is None:
            test_fail("Expected ABORT was not found during program run number {} of {}".format(k, repetitions))
        elif res[2] > 1:
            # Ignore non-error messages
            break


sys.exit(0)
