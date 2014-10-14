#!/usr/bin/env python
from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_raises, assert_equal, assert_greater, \
    assert_in, assert_not_in

from machinekit import hal
import time

@test(groups=["hal","hal_ring_base"],
      depends_on_groups=["hal_base","rtapi_base"])
class TestRingCmd(RTAPITestCase):

    @before_class
    def setup_checks(self):
        """Ring:  Test environment is clean"""
        # check the ring doesn't already exist from a previous test
        assert_not_in("ring1",hal.rings())
        assert_not_in("ring2",hal.rings())

    @test
    def attach_nonexistent_ring(self):
        """Ring:  Attaching nonexistent ring raises exception"""
        assert_raises(NameError, hal.Ring, "ring2")

    @test
    def create_new_ring(self):
        """Ring:  Create ring"""
        # size given mean - create existing ring
        self.r = hal.Ring("ring1", size=4096)
        assert_equal(len(hal.rings()),1)
        assert_in("ring1",hal.rings())

    @test
    def create_existing_ring(self):
        """Ring:  Create existing ring raises exception"""
        hal.Ring("ring2", size=4096)
        assert_raises(RuntimeError, hal.Ring, "ring2", size=4096)
        hal.Ring.delete("ring2")
        assert_not_in("ring2", hal.rings())

    @test(depends_on=[create_new_ring])
    def loadrt_ringwrite(self):
        """Ring:  loadrt and start ringwrite thread"""
        self.rtapi.loadrt("ringwrite","ring=ring1")
        self.rtapi.newthread("servo-thread",1000000,use_fp=True)
        hal.addf("ringwrite","servo-thread")
        hal.start_threads()
        time.sleep(1) # let rt thread write a bit to ring

    @test(depends_on=[loadrt_ringwrite])
    def wiggle_write(self):
        """Ring:  Wiggle pin"""
        p = hal.Pin("ringwrite.write")
        for n in range(10):
            p.set(not p.get())
            # triggered thread execution: urgently needed
            time.sleep(0.1)

    @test(depends_on=[wiggle_write])
    def ring_read(self):
        """Ring:  Read ring"""
        nr = 0
        for n in range(10):
            time.sleep(0.1)
            record = self.r.read()
            if record is None:
                break
            print "consume record %d: '%s'" % (nr, record)
            nr += 1
            self.r.shift()
        assert_greater(nr, 0)

    @test(depends_on=[ring_read])
    def unloadrt_ringwrite(self):
        """Ring:  Stop and unloadrt ringwrite thread"""
        # unload the ringwrite component to deref ring1
        hal.stop_threads()
        hal.delf("ringwrite","servo-thread")
        self.rtapi.delthread("servo-thread")
        self.rtapi.unloadrt("ringwrite")

    @test(depends_on=[unloadrt_ringwrite])
    def delete_ring(self):
        """Ring:  Delete ring"""
        self.r = None  # remove reference
        hal.Ring.delete("ring1")
        assert_not_in("ring1",hal.rings())

    @after_class
    def cleanup(self):
        """Ring:  Clean up"""
        check_hal_clean()
