#!/usr/bin/env python

import linuxcnc
import time
import sys

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


c = linuxcnc.command()
s = linuxcnc.stat()
e = linuxcnc.error_channel()

# Wait for LinuxCNC to initialize itself so the Status buffer stabilizes.
wait_for_linuxcnc_startup(s)

# Reset and home machine
c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.home(-1)
c.wait_complete()

def do_unit_checks(s):
    # Print unit checks
    # - Header
    print "CHECK %s at line %d:  gcodes[0]=%d" % \
        ("Paused" if (s.interp_state==linuxcnc.INTERP_PAUSED) else "Not paused",
         s.current_line, s.gcodes[0])
    # - Sanity checks
    if (s.current_line) != s.gcodes[0]:
        print "CHECK ERROR:  s.current_line does not match s.gcodes"

def wait_for_pause():
    done = False
    while True:
        # Wait for program to reach either paused or idle state
        timeout=0.5 # seconds
        while timeout > 0.0:
            s.poll()
            if s.interp_state == linuxcnc.INTERP_PAUSED:
                break
            timeout -= 0.1
            time.sleep(0.1)
        else:
            # Timeout:  last case is long motion where we're expected
            # to abort
            print "CHECK hit timeout (expected during motion); aborting"
            print "CHECK    Pre-abort checks:"
            do_unit_checks(s)
            c.abort()
            c.wait_complete()
            s.poll()
            print "CHECK    Post-abort checks:"
            do_unit_checks(s)
            print "CHECK Done; exiting"
            return

        # Print unit checks and resume program
        do_unit_checks(s)
        c.auto(linuxcnc.AUTO_RESUME)
        c.wait_complete()

# Start program
c.mode(linuxcnc.MODE_AUTO)
c.program_open('test.ngc')
c.auto(linuxcnc.AUTO_RUN, 1)
c.wait_complete()

# Loop through pause/resume, printing unit checks
wait_for_pause()

sys.exit(0)
