#!/usr/bin/env python

import linuxcnc, hal
import sys, os, time


# Set up component
h = hal.component("python-ui")
# - Monitor spindle and Z axis speed for debugging
h.newpin("spindle-speed", hal.HAL_FLOAT, hal.HAL_IN)
h.newpin("Zpos", hal.HAL_FLOAT, hal.HAL_IN)
# - Monitor waiting-on-spindle pin
h.newpin("waiting-on-spindle", hal.HAL_BIT, hal.HAL_IN)
# - Mark the component as 'ready'
h.ready()
# - Net component to rest of HAL config
os.system("halcmd source ./postgui.hal")


# Initialization
c = linuxcnc.command()
s = linuxcnc.stat()
c.state(linuxcnc.STATE_ESTOP_RESET)
c.state(linuxcnc.STATE_ON)
c.mode(linuxcnc.MODE_MDI)
s.poll()

# Check waiting_on_spindle
print "After init:  waiting_on_spindle:  stat = %s; HAL = %s" % (
        s.waiting_on_spindle, h['waiting-on-spindle'])

# Spindle:  set speed and start
c.mdi('S750 M3')
c.wait_complete()
time.sleep(0.2)
    
def test_motion(cmd, msg, wos_state):
    print msg

    # Params for waiting for spindle
    time_incr = 0.1 # Seconds between checks
    time_error = 5  # Seconds to wait before aborting

    print "    Before spindle override = 0:  " \
        "waiting_on_spindle:  stat = %s; HAL = %s; spindle speed = %.3f" % (
            s.waiting_on_spindle, h['waiting-on-spindle'], h['spindle-speed'])
    print "    Zpos = %.3f" % h['Zpos']
    if s.waiting_on_spindle or h['waiting-on-spindle']:
        print "*** waiting_on_spindle unexpectedly asserted before test:"
        print "***    stat = %s;  HAL=%s" % (
            s.waiting_on_spindle, h['waiting-on-spindle'])
        return 1

    # Set spindle override to zero and wait for it to stop
    print "    Zeroing spindle override"
    c.spindleoverride(0.0)
    spindle_time = 0.0
    while spindle_time < time_error and h['spindle-speed'] > 0.001:
        s.poll()
        print "        After %.1fs:  spindle_speed = %.3f" % (
            spindle_time, h['spindle-speed'])
        print "            waiting_on_spindle:  stat = %s; HAL = %s" % (
            s.waiting_on_spindle, h['waiting-on-spindle'])
        spindle_time += time_incr
        time.sleep(time_incr)
    if spindle_time >= time_error or h['spindle-speed'] > 0.001:
        print "*** Spindle failed to stop in time " \
            "(timeout=%.1f; spindle-speed=%.3f)" % (
                spindle_time, h['spindle-speed'])
        return 1
    print "        Spindle is stopped:  %.3f" % h['spindle-speed']

    # Start command
    print "    Running command '%s'" % cmd
    c.mdi(cmd)
    c.wait_complete()
    s.poll()

    # Check waiting_on_spindle
    print "    After cmd:  waiting_on_spindle:  stat = %s; HAL = %s; " \
        "spindle speed = %.3f" % (
            s.waiting_on_spindle, h['waiting-on-spindle'], h['spindle-speed'])

    # Start spindle and continue checks
    print "    Resetting spindle override"
    c.spindleoverride(1.0)
    spindle_time = 0.0
    while spindle_time < time_error and s.waiting_on_spindle is not wos_state:
        s.poll()
        print "        After %.1fs:  spindle_speed:  stat = %.1f" % (
            spindle_time, h['spindle-speed'])
        print "            waiting_on_spindle:  stat = %s; HAL = %s" % (
            s.waiting_on_spindle, h['waiting-on-spindle'])
        spindle_time += time_incr
        time.sleep(time_incr)
    # Error out if spindle never got up to speed
    if spindle_time >= time_error:
        print "*** waiting_on_spindle never asserted within time limit"
        return 1
    print "    waiting_on_spindle:  stat = %s; HAL = %s; " \
        "spindle speed = %.3f" % (
            s.waiting_on_spindle, h['waiting-on-spindle'], h['spindle-speed'])

    # Let motion continue a little and abort command
    time.sleep(0.2)
    c.abort()
    c.wait_complete()
    s.poll()

    # Check waiting_on_spindle
    print "    After abort:  waiting_on_spindle:  stat = %s; HAL = %s; " \
        "spindle speed = %.3f" % (
            s.waiting_on_spindle, h['waiting-on-spindle'], h['spindle-speed'])
    if s.waiting_on_spindle or h['waiting-on-spindle']:
        print "*** waiting_on_spindle unexpectedly asserted after abort:"
        print "***    stat = %s;  HAL=%s" % (
            s.waiting_on_spindle, h['waiting-on-spindle'])
        return 1
    return 0

res = 0
# waiting_on_spindle should be asserted during G33
res += test_motion('G33 Z-5 K0.01', 'G33 spindle-synchronized move', True)
# waiting_on_spindle should be not asserted during G1
res += test_motion('G1 Z-5 F0.01', 'G1 unsynchronized move', False)


print "Final Zpos = %.3f" % h['Zpos']
sys.exit(res)
