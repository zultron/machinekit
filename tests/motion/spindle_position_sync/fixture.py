#!/usr/bin/env python

import linuxcnc
import sys
from subprocess import Popen

import g33_checks

# Initialize linuxcnc backend for MDI mode
c = linuxcnc.command()
s = linuxcnc.stat()
s.poll()
c.state(linuxcnc.STATE_ESTOP_RESET)
c.wait_complete()
c.state(linuxcnc.STATE_ON)
c.wait_complete()
c.mode(linuxcnc.MODE_MDI)
c.wait_complete()

# Read in the current test case (passed to the fixture via sys args)
test_name = sys.argv[3]
constants = g33_checks.load_test_case(test_name)
print >> sys.stderr, "Test {} configuration: {}".format(test_name,constants)

if constants.get("skip", False):
    print >> sys.stderr, "Test {} skipped".format(test_name)
    sys.exit(0)

# Start halsampler asynchronously to make sure that the entire test is captured
sample_file = g33_checks.get_sample_file_name(test_name)
print >> sys.stderr, "Starting halsampler writing to ", sample_file
p_read = Popen([
    'halsampler',
    '-t',
    sample_file,
])

print >> sys.stderr, "start spindle"
c.mdi('S%d %s' % (constants["spindle_speed_rps"]*60., g33_checks.get_spindle_mcode(constants)))
c.wait_complete(10)
print >> sys.stderr, "tap move"
cmd_word = 'G33.1' if constants.get("rigid_tap", True) else 'G33'
z_pecks = max(int(constants.get("z_pecks",1)),1)
z_delta = constants["z_distance"] / float(z_pecks)
for peck in range(1,z_pecks+1):
    cmd = '{} Z-{} K{}'.format(cmd_word, z_delta * peck, constants["pitch"])
    print >>sys.stderr, cmd
    c.mdi(cmd)
c.wait_complete(100)
c.mdi('G4 P0.01')
c.wait_complete()
p_read.terminate()
p_read.wait()
print >> sys.stderr, "halsampler done with code %d" % p_read.returncode

c.wait_complete()
failed = g33_checks.check_all(test_name)

if failed:
    print >> sys.stderr, "Test {} failed".format(test_name)

