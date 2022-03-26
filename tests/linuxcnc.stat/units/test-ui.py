#!/usr/bin/env python

import linuxcnc
import time
import sys
from testing.linuxcnc_test_support import wait_for_linuxcnc_startup


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
    units_map = {
        linuxcnc.UNITS_INCHES : "UNITS_INCHES",
        linuxcnc.UNITS_MM : "UNITS_MM",
    }
    # - Header
    print("CHECK %s at line %d:  units %d=%s; gcodes[5]=%d" % \
        ("Paused" if (s.interp_state==linuxcnc.INTERP_PAUSED) else "Not paused",
         s.current_line,
         s.program_units,
         units_map.get(s.program_units, 'unknown'),
         s.gcodes[5]))
    # - Sanity checks
    if (s.program_units*10+190) != s.gcodes[5]:
        print("CHECK ERROR:  s.program_units does not match s.gcodes[5]")

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
            print("CHECK hit timeout (expected during motion); aborting")
            print("CHECK    Pre-abort checks:")
            do_unit_checks(s)
            c.abort()
            c.wait_complete()
            s.poll()
            print("CHECK    Post-abort checks:")
            do_unit_checks(s)
            print("CHECK Done; exiting")
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
