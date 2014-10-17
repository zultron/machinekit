#!/usr/bin/env python

from utils import RTAPITestCase
from nose.tools import assert_equal, assert_true, assert_false

from machinekit import hal
import time

class test_030_comp_or2(RTAPITestCase):

    def test_03010_loadrt(self):
        "03010 comp or2:  loadrt and start threads"
        self.rtapi.loadrt("or2")
        self.rtapi.newthread("servo-thread",1000000,use_fp=True)
        hal.addf("or2.0","servo-thread")
        hal.start_threads()

    def test_03020_pin_properties(self):
        "03020 comp or2:  pin properties"
        in0 = hal.Pin("or2.0.in0")
        assert_equal(in0.type, hal.HAL_BIT)
        assert_equal(in0.dir, hal.HAL_IN)
        assert_false(in0.linked)

        in1 = hal.Pin("or2.0.in1")
        assert_equal(in1.type, hal.HAL_BIT)
        assert_equal(in1.dir, hal.HAL_IN)
        assert_false(in1.linked)

        out = hal.Pin("or2.0.out")
        assert_equal(out.type, hal.HAL_BIT)
        assert_equal(out.dir, hal.HAL_OUT)
        assert_false(out.linked)

        # unlinked default values
        assert_false(in0.get())
        assert_false(in1.get())

        # output value at default value
        assert_false(out.get())

        in0.set(True)
        time.sleep(0.1) # we urgently need a 1-shot thread function!
        assert_true(out.get())

        in0.set(False)
        time.sleep(0.1)
        assert_false(out.get())

        in1.set(True)
        time.sleep(0.1)
        assert_true(out.get())

        in1.set(False)
        time.sleep(0.1)
        assert_false(out.get())

        in1.set(True)
        in0.set(True)
        time.sleep(0.1)
        assert_true(out.get())


    def test_03090_unloadrt(self):
        """03090 comp or2:  clean up"""
        hal.stop_threads()
        hal.delf("or2.0","servo-thread")
        self.rtapi.delthread("servo-thread")
        self.rtapi.unloadrt("or2")
