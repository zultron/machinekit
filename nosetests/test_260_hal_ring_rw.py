# create a ring
# assure records written can be read back

from . import RTAPITestCase
from nose.tools import assert_equal, assert_is_none, assert_is_not_none

from machinekit import hal

class test_250_hal_ring_rw(RTAPITestCase):

    def test_25010_init_ring(self):
        """25010 hal ring r/w:  Initialize ring"""
        # size given mean - create existing ring
        self.f.r = hal.Ring("ring1", size=4096)
        # leave around - reused below

    def test_25020_ring_write_read(self):
        """25020 hal ring r/w:  Write to and read from ring"""
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

    def test_25090_delete_ring(self):
        """25090 hal ring r/w:  Delete ring"""
        self.f.r = None  # remove refcount
        hal.Ring.delete("ring1")
        assert_equal(len(hal.rings()),0)
