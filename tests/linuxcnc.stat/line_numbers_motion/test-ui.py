#!/usr/bin/env python

import linuxcnc, hal
import sys, os, time


def wait_for_linuxcnc_startup(status, timeout=10.0):

    """Poll the Status buffer waiting for it to look initialized,
    rather than just allocated (all-zero).  Returns on success, throws
    RuntimeError on failure."""

    start_time = time.time()
    while time.time() - start_time < timeout:
        status.poll()
        if (status.angular_units == 0.0) \
            or (status.axes == 0) \
            or (status.axis_mask == 0) \
            or (status.cycle_time == 0.0) \
            or (status.exec_state != linuxcnc.EXEC_DONE) \
            or (status.interp_state != linuxcnc.INTERP_IDLE) \
            or (status.inpos == False) \
            or (status.linear_units == 0.0) \
            or (status.max_acceleration == 0.0) \
            or (status.max_velocity == 0.0) \
            or (status.program_units == 0.0) \
            or (status.rapidrate == 0.0) \
            or (status.state != linuxcnc.STATE_ESTOP) \
            or (status.task_state != linuxcnc.STATE_ESTOP):
            time.sleep(0.1)
        else:
            # looks good
            return

    # timeout, throw an exception
    raise RuntimeError

# Set up component
h = hal.component("python-ui")
# - Monitor X axis position to detect current move
h.newpin("Xpos", hal.HAL_FLOAT, hal.HAL_IN)
h.ready() # mark the component as 'ready'
os.system("halcmd source ./postgui.hal")

# Initialization
c = linuxcnc.command()
s = linuxcnc.stat()

# Wait for LinuxCNC to initialize itself so the Status buffer stabilizes.
wait_for_linuxcnc_startup(s)

# Reset and home machine
c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.home(-1)
c.wait_complete()

# Start program
c.mode(linuxcnc.MODE_AUTO)
c.program_open('test.ngc')
c.auto(linuxcnc.AUTO_RUN, 1)

# Wait for X axis to reach test position
goal = 0.01 # X position signaling test
timeout = 5 # seconds to wait before erroring out
time_elapsed = 0 # elapsed time
time_incr = 0.1 # how long to sleep
while time_elapsed < timeout:
    print "Current Xpos = %.3f; elapsed = %.1f" % (h['Xpos'], time_elapsed)
    if abs(h['Xpos'] - goal) < 0.0001:
        print "Reached test position"
        break
    time_elapsed += time_incr
    time.sleep(time_incr)
else:
    print "Error:  timeout exceeded waiting for X pos to reach %.2f" % goal
    sys.exit(1)

# Check that line number is 8
s.poll()
if s.current_line < 8:
    print "Error:  line number %d < 8" % s.current_line
    sys.exit(1)
if s.current_line > 8:
    print "Error:  line number %d > 8" % s.current_line
    sys.exit(1)
print "Ok:  line number %d" % s.current_line

# Abort and exit
c.abort()
c.wait_complete()
sys.exit(0)
