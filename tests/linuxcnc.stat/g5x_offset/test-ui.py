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
    offsets_map = dict(enumerate(
        ['cur'] +
        [ i*10 + 540 for i in range(6)] +
        [ i+591 for i in range(3) ] +
        [ i*20 + 10001 for i in range(10,500)]))
    # - Header
    print "CHECK %s at line %d:  offset %d/%s" % \
        ("Paused" if (s.interp_state==linuxcnc.INTERP_PAUSED) else "Not paused",
         s.current_line,
         s.g5x_index,
         offsets_map.get(s.g5x_index, 'unknown'))
    # - Active modes
    print "CHECK gcodes:  %s" % (s.gcodes,)
    # - Current + all offsets in both user and program units
    standard_offsets = [ (k, s.g5x_offsets[k]) for k in range(10)]
    for i, o in [(0, s.g5x_offset)] + standard_offsets:
        print "CHECK offset %d/%s:  X%.2f Y%.2f Z%.2f" % \
            tuple([i, offsets_map[i]] + list(o[:3]))
    # - Sanity checks
    if offsets_map.get(s.g5x_index, None) != s.gcodes[8]:
        print "CHECK ERROR:  s.g5x_index=%s/%s != s.gcodes[8]=%s" % \
            (s.g5x_index, offsets_map.get(s.g5x_index, None), s.gcodes[8])

def wait_for_pause():
    done = False
    while True:
        sys.stderr.write("waiting for pause\n")
        # Wait for program to reach either paused or idle state
        timeout=0.5 # seconds
        while timeout > 0.0:
            s.poll()
            if s.interp_state == linuxcnc.INTERP_PAUSED:
                break
            timeout -= 0.1
            time.sleep(0.1)
            sys.stderr.write("waiting for timeout\n")
        else:
            sys.stderr.write("do unit checks\n")
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
        sys.stderr.write("do unit checks\n")
        do_unit_checks(s)
        c.auto(linuxcnc.AUTO_RESUME)
        c.wait_complete()

# Start program
sys.stderr.write("starting program\n")
c.mode(linuxcnc.MODE_AUTO)
c.program_open('test.ngc')
c.auto(linuxcnc.AUTO_RUN, 1)
c.wait_complete()

# Loop through pause/resume, printing unit checks
wait_for_pause()

sys.exit(0)
