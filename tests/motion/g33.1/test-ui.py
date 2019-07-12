#!/usr/bin/env python

import linuxcnc
import sys
from subprocess import Popen

import g33_checks

# Initialization
c = linuxcnc.command()
s = linuxcnc.stat()
s.poll()
c.state(linuxcnc.STATE_ESTOP_RESET)
c.wait_complete()
c.state(linuxcnc.STATE_ON)
c.wait_complete()
c.mode(linuxcnc.MODE_MDI)
c.wait_complete()

test_name = sys.argv[3]
constants = g33_checks.load_constants(test_name)
print >> sys.stderr, "Got constants: ", constants

# Spindle: set speed and start
print >> sys.stderr, "Starting halsampler..."
p_read = Popen([
    'halsampler',
    '-t',
    '{}_samples'.format(test_name),
])
print >> sys.stderr, "start spindle"
c.mdi('S%d M3' % (constants["spindle_speed_rps"]*60.))
c.wait_complete()
print >> sys.stderr, "tap move"
c.mdi('G33.1 Z-{} K{}'.format(constants["z_distance"], constants["pitch"]))
c.wait_complete()
c.mdi('G4 P0.01')
c.wait_complete()
p_read.terminate()
p_read.wait()
print >> sys.stderr, "halsampler done with code %d" % p_read.returncode

c.wait_complete()
failed = g33_checks.check_all(test_name)

if failed:
    sys.exit(1)

