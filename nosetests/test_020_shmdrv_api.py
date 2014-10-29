from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_almost_equal, assert_not_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi import shmdrv_api
import os

class test_020_shmdrv_api(FixtureTestCase):

    def test_02010_shmdrv_api_setup(self):
        """02010 shmdrv_api:  shmdrv_api setup and init"""

        self.fix(
            key1 = 0x00badbad,
            size1 = 512,
            key2 = 0x00b00dad,
            size2 = 1024,
            prefix = "/nosetest-",
            )
        self.fix(
            posix_name = lambda junk,key: "%s%08x" % (self.prefix, key),
            )

        # Init and set prefix
        shmdrv_api.init()
        shmdrv_api.set_name_format(self.prefix)

        # Clean up any leftover segs from past runs
        for key in (self.key1, self.key2):
            if shmdrv_api.exists(key):
                self.log.warn("Unlinking existing shm seg, key=%08d" % key)
                shmdrv_api.SHMSegment().attach(key).unlink()

    def test_02020_uninitialized_seg_attributes(self):
        """02020 shmdrv_api:  uninitialized seg attributes are None"""

        seg = shmdrv_api.SHMSegment()
        # Attributes start out as None
        assert_equal(seg.key, 0)
        assert_equal(seg.size, 0)
        assert_is_none(seg.posix_name)
        assert_equal(seg.ptr, 0)

    def test_02021_create_invalid_key(self):
        """02021 shmdrv_api:  new seg with invalid key raises exception"""

        seg = shmdrv_api.SHMSegment()
        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      seg.new, 0, self.size1)

    def test_02040_new_shm_seg(self):
        """02040 shmdrv_api:  new shm segment"""

        # make it a fixture for later
        self.fix(seg1 = shmdrv_api.SHMSegment().new(self.key1, self.size1))


    def test_02041_shm_seg_attributes(self):
        """02041 shmdrv_api:  shm segment attributes"""

        seg = self.seg1
        assert_equal(seg.key, self.key1)
        assert_equal(seg.size, self.size1)
        assert_equal(seg.posix_name, self.posix_name(self.key1))
        assert_is_not_none(seg.ptr)

    def test_02042_shm_seg_file(self):
        """02042 shmdrv_api:  shm seg has file in /dev/shm"""

        assert_true(os.path.exists("/dev/shm/%s" % self.seg1.posix_name))

    def test_02043_shm_seg_exists(self):
        """02043 shmdrv_api:  shm seg exists() functions"""

        assert_true(shmdrv_api.exists(self.key1))
        assert_true(self.seg1.exists())

    def test_02044_create_existing_shm_seg(self):
        """02044 shmdrv_api:  initialized_obj.new() raises exception"""

        # sanity
        assert_equal(self.seg1.key, self.key1)

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      self.seg1.new,self.key1,self.size1)

    def test_02050_shm_attach(self):
        """02050 shmdrv_api:  attach to existing shm segment"""

        seg = shmdrv_api.SHMSegment().attach(self.key1)
        assert_equal(seg.posix_name, self.seg1.posix_name)

        # make object a fixture
        self.fix(seg1_clone = seg)

    def test_02060_second_segment(self):
        """02060 shmdrv_api:  create a second segment and check it"""

        seg2 = shmdrv_api.SHMSegment().new(self.key2, self.size2)
        # make obj a fixture
        self.fix(seg2 = seg2)

        # check attributes
        assert_equal(seg2.key, self.key2)
        assert_equal(seg2.size, self.size2)
        assert_equal(seg2.posix_name, self.posix_name(self.key2))

    def test_02061_two_segments_no_clash(self):
        """02061 shmdrv_api:  two segments do not clash"""

        # check the first segment's attributes
        assert_equal(self.seg1.key, self.key1)
        assert_equal(self.seg1.size, self.size1)
        assert_equal(self.seg1.posix_name, self.posix_name(self.key1))

        # check the two segments attributes not equal
        assert_not_equal(self.seg1.key, self.seg2.key)
        assert_not_equal(self.seg1.size, self.seg2.size)
        assert_not_equal(self.seg1.posix_name, self.seg2.posix_name)

    def test_02070_unlink_segment(self):
        """02070 shmdrv_api:  unlink a segment; check exists()"""

        self.seg2.unlink()
        assert_false(shmdrv_api.exists(self.key2))
        assert_false(self.seg2.exists())

    def test_02071_attach_unlinked_seg_fails(self):
        """02071 shmdrv_api:  attach unlinked segment fails"""

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      shmdrv_api.SHMSegment().attach, self.key2)

    def test_02072_create_new_existing_seg_fails(self):
        """02072 shmdrv_api:  create new existing segment fails"""

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      shmdrv_api.SHMSegment().new, self.key1, self.size1)

    def test_02073_unlink_last_segment(self):
        """02073 shmdrv_api:  unlink last segment"""

        self.seg1.unlink()
        assert_false(shmdrv_api.exists(self.key1))
        assert_false(os.path.exists("/dev/shm/%s" % self.posix_name(self.key1)))
        assert_false(os.path.exists("/dev/shm/%s" % self.posix_name(self.key2)))

    @skip("shmdrvapi.c:  no error unlinking unlinked segment")
    def test_02074_unlink_unlinked_segment_fails(self):
        """02074 shmdrv_api:  unlinking unlinked segment fails"""
        # This one doesn't raise an error like one might expect.  If
        # this is intended, this can be safely removed.

        assert_false(shmdrv_api.exists(self.key1))
        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      self.seg1.unlink)
