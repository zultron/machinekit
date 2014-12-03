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

    def test_02020_seg_attribute_init(self):
        """02020 shmdrv_api:  seg attributes initialization"""

        seg = shmdrv_api.SHMSegment(self.key1, self.size1)
        # Attributes initialized properly
        assert_equal(seg.key, self.key1)
        assert_equal(seg.size, self.size1)
        assert_equal(seg.ptr, 0)

    @skip("Unwritten")
    def test_02022_set_name_format(self):
        """02022 shmdrv_api:  set_name_format()"""
        pass


    def test_02030_new_shm_seg(self):
        """02030 shmdrv_api:  new shm segment"""

        # make it a fixture for later
        self.fix(seg1 = shmdrv_api.SHMSegment(self.key1, self.size1).new())


    def test_02031_shm_seg_attributes(self):
        """02031 shmdrv_api:  shm segment attributes"""

        seg = self.seg1
        assert_equal(seg.key, self.key1)
        assert_equal(seg.size, self.size1)
        assert_equal(seg.posix_name, self.posix_name(self.key1))
        assert_is_not_none(seg.ptr)

    def test_02032_shm_seg_file(self):
        """02032 shmdrv_api:  shm seg has file in /dev/shm"""

        assert_true(os.path.exists("/dev/shm/%s" % self.seg1.posix_name))

    def test_02033_shm_seg_exists(self):
        """02033 shmdrv_api:  shm seg exists() functions"""

        assert_true(shmdrv_api.exists(self.key1))
        assert_true(self.seg1.exists())

    def test_02034_create_existing_shm_seg(self):
        """02034 shmdrv_api:  initialized_obj.new() raises exception"""

        # sanity
        assert_equal(self.seg1.key, self.key1)

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      self.seg1.new)

    def test_02040_shm_attach(self):
        """02040 shmdrv_api:  attach to existing shm segment"""

        seg = shmdrv_api.SHMSegment(self.key1).attach()
        assert_equal(seg.posix_name, self.seg1.posix_name)

        # make object a fixture
        self.fix(seg1_clone = seg)

    def test_02050_second_segment(self):
        """02050 shmdrv_api:  create a second segment and check it"""

        seg2 = shmdrv_api.SHMSegment(self.key2, self.size2).new()
        # make obj a fixture
        self.fix(seg2 = seg2)

        # check attributes
        assert_equal(seg2.key, self.key2)
        assert_equal(seg2.size, self.size2)
        assert_equal(seg2.posix_name, self.posix_name(self.key2))

    def test_02051_two_segments_no_clash(self):
        """02051 shmdrv_api:  two segments do not clash"""

        # check the first segment's attributes
        assert_equal(self.seg1.key, self.key1)
        assert_equal(self.seg1.size, self.size1)
        assert_equal(self.seg1.posix_name, self.posix_name(self.key1))

        # check the two segments attributes not equal
        assert_not_equal(self.seg1.key, self.seg2.key)
        assert_not_equal(self.seg1.size, self.seg2.size)
        assert_not_equal(self.seg1.posix_name, self.seg2.posix_name)

    def test_02060_unlink_segment(self):
        """02060 shmdrv_api:  unlink a segment; check exists()"""

        self.seg2.unlink()
        assert_false(shmdrv_api.exists(self.key2))
        assert_false(self.seg2.exists())

    def test_02061_attach_unlinked_seg_fails(self):
        """02061 shmdrv_api:  attach unlinked segment fails"""

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      shmdrv_api.SHMSegment(self.key2).attach)

    def test_02062_create_new_existing_seg_fails(self):
        """02062 shmdrv_api:  create new existing segment fails"""

        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      shmdrv_api.SHMSegment(self.key1, self.size1).new)

    def test_02063_unlink_last_segment(self):
        """02063 shmdrv_api:  unlink last segment"""

        self.seg1.unlink()
        assert_false(shmdrv_api.exists(self.key1))
        assert_false(os.path.exists("/dev/shm/%s" % self.posix_name(self.key1)))
        assert_false(os.path.exists("/dev/shm/%s" % self.posix_name(self.key2)))

    def test_02064_unlink_unlinked_segment_fails(self):
        """02064 shmdrv_api:  unlinking unlinked segment fails"""
        # This one doesn't raise an error like one might expect.  If
        # this is intended, this can be safely removed.

        assert_false(shmdrv_api.exists(self.key1))
        assert_raises(shmdrv_api.SHMDrvAPIRuntimeError,
                      self.seg1.unlink)


    @skip("Unwritten")
    def test_02070_detach_segment(self):
        """02070 shmdrv_api:  detach_segment()"""
        pass


    @skip("Unwritten")
    def test_02080_shmdrv_available(self):
        """02080 shmdrv_api:  shmdrv_available()"""
        pass

    @skip("Unwritten")
    def test_02081_shmdrv_gc(self):
        """02081 shmdrv_api:  shmdrv_gc()"""
        pass

    @skip("Unwritten")
    def test_02082_shmdrv_loaded(self):
        """02082 shmdrv_api:  shmdrv_loaded()"""
        pass

