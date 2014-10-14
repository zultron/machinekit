#!/usr/bin/env python

# create a ring
# assure records written can be read back

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_equal, assert_not_in

from machinekit import hal

@test(groups=["hal"],
      depends_on_groups=["hal_ring_rw","rtapi_base"])
class TestStreamringCmd(RTAPITestCase):

    size=4096

    @before_class
    def init_streamring(self):
        """Stream Ring:  Initialize stream ring """
        # the ring needs to have a reference to it maintained or it
        # will be garbage collected, and the next test will result in
        # a segfault
        self.r = hal.Ring("ring1", size=self.size, type=hal.RINGTYPE_STREAM)
        self.sr = hal.StreamRing(self.r)

    @test
    def ring_write_read(self):
        """Stream Ring:  Write to and read from stream ring"""
        for n in range(self.size):
            if self.sr.write("X") < 1:
                assert_equal(n, self.size - 1)

        m = self.sr.read()
        assert_equal(len(m), self.size - 1)

    @after_class
    def teardown_class(self):
        """Stream Ring:  Remove stream ring"""
        # remove refs to ring
        self.sr = None
        self.r = None

        hal.Ring.delete("ring1")
        assert_not_in("ring1",hal.rings(),
                      "Failed to delete ring 'ring1'; rings: %s" % hal.rings())

        check_hal_clean()

