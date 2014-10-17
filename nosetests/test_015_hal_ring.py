#!/usr/bin/env python
from utils import RTAPITestCase
from nose.tools import assert_raises, assert_equal, assert_greater, \
    assert_in, assert_not_in

from machinekit import hal
import time

class test_015_hal_ring(RTAPITestCase):

    def test_01510_create_new_ring(self):
        """01510 hal ring:  Create ring"""
        # size given mean - create existing ring
        self.f.r = hal.Ring("ring1", size=4096)
        assert_equal(len(hal.rings()),1)
        assert_in("ring1",hal.rings())

    def test_01520_attach_nonexistent_ring(self):
        """01520 hal ring:  Attaching nonexistent ring raises exception"""
        assert_raises(NameError, hal.Ring, "ring2")

    def test_01521_create_existing_ring(self):
        """01521 hal ring:  Create existing ring raises exception"""
        hal.Ring("ring2", size=4096)
        assert_raises(RuntimeError, hal.Ring, "ring2", size=4096)
        hal.Ring.delete("ring2")
        assert_not_in("ring2", hal.rings())

    def test_01530_loadrt_ringwrite(self):
        """01530 hal ring:  loadrt and start ringwrite thread"""
        self.rtapi.loadrt("ringwrite","ring=ring1")
        self.rtapi.newthread("servo-thread",1000000,use_fp=True)
        hal.addf("ringwrite","servo-thread")
        hal.start_threads()
        time.sleep(1) # let rt thread write a bit to ring

    def test_01540_wiggle_write(self):
        """01540 hal ring:  Wiggle pin"""
        p = hal.Pin("ringwrite.write")
        for n in range(10):
            p.set(not p.get())
            # triggered thread execution: urgently needed
            time.sleep(0.1)

    def test_01550_ring_read(self):
        """01550 hal ring:  Read ring"""
        nr = 0
        for n in range(10):
            time.sleep(0.1)
            record = self.f.r.read()
            if record is None:
                break
            print "consume record %d: '%s'" % (nr, record)
            nr += 1
            self.f.r.shift()
        assert_greater(nr, 0)

    def test_01590_unloadrt_ringwrite(self):
        """01590 hal ring:  Stop and unloadrt ringwrite thread"""
        # unload the ringwrite component to deref ring1
        hal.stop_threads()
        hal.delf("ringwrite","servo-thread")
        self.rtapi.delthread("servo-thread")
        self.rtapi.unloadrt("ringwrite")

    def test_01591_cleanup(self):
        """01591 hal ring:  Remove ring"""
        assert_in("ring1",hal.rings())
        self.f.r = None  # remove reference
        hal.Ring.delete("ring1")
