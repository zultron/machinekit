#!/usr/bin/env python

# create a ring
# assure records written can be read back

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_equal, assert_is_none, assert_is_not_none

from machinekit import hal

@test(groups=["hal","hal_ring_rw"],
      depends_on_groups=["hal_ring","rtapi"])
class TestRingIntracompCmd(RTAPITestCase):

    @before_class
    def init_ring(self):
        """Ring Read/Write:  Initialize ring"""
        # size given mean - create existing ring
        self.r = hal.Ring("ring1", size=4096)
        # leave around - reused below

    @test
    def ring_write_read(self):
        """Ring Read/Write:  Write to and read from ring"""
        r = hal.Ring('ring1')
        nr = 0
        count = 100
        for n in range(count):
            r.write("record %d" % n)
            record = r.read()
            assert_is_not_none(record,"no record after write %d" % n)
            nr += 1
            r.shift()
        assert_equal(nr, count)
        record = r.read()
        assert_is_none(record) # ring must be empty

    @after_class(always_run=True)
    def delete_ring(self):
        """Ring Read/Write:  Delete ring"""
        self.r = None  # remove refcount
        hal.Ring.delete("ring1")
        assert_equal(len(hal.rings()),0)

        check_hal_clean()
