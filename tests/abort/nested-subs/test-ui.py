#!/usr/bin/env python

import linuxcnc
import time, sys, os

#
# INIT
#
c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.home(-1)
c.wait_complete()

#
# TEST
#

# Run program
print "Running program 'test.ngc'"
c.mode(linuxcnc.MODE_AUTO)
c.program_open("test.ngc")
c.auto(linuxcnc.AUTO_RUN, 1)

# Wait for things to get underway, then abort
c.wait_complete()
# Let zig-zags run to a certain point
while s.position[0] == 0.0:
    time.sleep(0.1)
    s.poll()
    print "X position = %.3f" % s.position[1]
c.abort()
c.wait_complete()


#
# CLEANUP
#
os.unlink('sim.var')
os.unlink('sim.var.bak')
sys.exit(1)
