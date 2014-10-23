# create a ring
# assure records written can be read back

from . import RTAPITestCase
from nose.tools import assert_equal, assert_not_in

from machinekit import hal

class test_270_hal_streamring(RTAPITestCase):

    size=4096

    def test_27010_init_streamring(self):
        """27010 hal streamring:  Initialize stream ring """
        # the ring needs to have a reference to it maintained or it
        # will be garbage collected, and the next test will result in
        # a segfault
        self.f.r = hal.Ring("ring1", size=self.size, type=hal.RINGTYPE_STREAM)
        self.f.sr = hal.StreamRing(self.f.r)

    def test_27020_ring_write_read(self):
        """27020 hal streamring:  Write to and read from stream ring"""
        for n in range(self.size):
            if self.f.sr.write("X") < 1:
                assert_equal(n, self.size - 1)

        m = self.f.sr.read()
        assert_equal(len(m), self.size - 1)

    def test_27090_remove_ring(self):
        """27090 hal streamring:  Remove stream ring"""
        # remove refs to ring
        self.f.sr = None
        self.f.r = None

        hal.Ring.delete("ring1")
        assert_not_in("ring1",hal.rings(),
                      "Failed to delete ring 'ring1'; rings: %s" % hal.rings())

