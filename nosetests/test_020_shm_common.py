from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi import shmcommon

import os, logging

@attr('config')
class test_020_rtapi_shm(FixtureTestCase):

    def test_02010_shm_common_setup(self):
        """02010 rtapi shm:  shm_common setup"""

        self.log.setLevel(logging.DEBUG)

        self.fix(key = 0x00badbad,
                 size = 512,
                 key2 = 0x00b00dad,
                 size2 = 1024,
                 prefix = "/nosetest-",
                 )

    def test_02020_shm_init_and_seg_name(self):
        """02020 rtapi shm:  shm init and check segment name"""

        shmcommon.init()
        assert_equal(shmcommon.segment_posix_name(self.key),
                     "/shmdrvapi_%08x" % self.key)

    def test_02021_shm_set_seg_format(self):
        """02021 rtapi shm:  set shm seg name format"""

        shmcommon.set_name_format(self.prefix)
        assert_equal(shmcommon.segment_posix_name(self.key),
                     "%s%08x" % (self.prefix,self.key))
        # make that a fixture
        self.fix(seg_name = shmcommon.segment_posix_name(self.key))

    def test_02030_clean_segs(self):
        """02030 rtapi shm:  clean any existing segs"""

        # clean up any existing seg
        try:
            tmpseg = shmcommon.attach(self.key)
            # no exception, so there's an existing segment
            self.log.warn("Cleaning up old left over segment %s" %
                          tmpseg.name)
            tmpseg.unlink()
        except:
            pass  # no seg existed; that's what we want

    def test_02040_new_shm_seg(self):
        """02040 rtapi shm:  create new shm segment"""

        # create a new test segment & verify it exists
        shmcommon.new(self.key, self.size)
        assert_true(os.path.exists("/dev/shm/%s" % self.seg_name))

    def test_02041_shm_seg_exists(self):
        """02041 rtapi shm:  shm seg exists"""

        assert_true(shmcommon.exists(self.key))

    def test_02050_shm_attach(self):
        """02050 rtapi shm:  attach to existing shm segment"""

        seg = shmcommon.attach(self.key)
        assert_equal(seg.name, self.seg_name)

        # make object a fixture
        self.add_fixture('seg', seg)

    def test_02060_segment_attributes(self):
        """02060 rtapi shm:  check shm segment attributes"""

        assert_equal(self.seg.key, self.key)
        assert_equal(self.seg.size, self.size)
        assert_equal(self.seg.name, self.seg_name)

    def test_02070_second_segment(self):
        """02070 rtapi shm:  create a second segment and check it"""

        seg2 = shmcommon.new(self.key2, self.size2)
        # make obj a fixture
        self.fix(seg2 = seg2)

        # check attributes
        assert_equal(seg2.key, self.key2)
        assert_equal(seg2.size, self.size2)
        
        # make name a fixture and check it
        self.fix(seg2_name = shmcommon.segment_posix_name(self.key2))
        assert_equal(seg2.name, self.seg2_name)

    def test_02071_two_segments_no_clash(self):
        """02071 rtapi shm:  two segments do not clash"""

        # check the first segment's attributes
        assert_equal(self.seg.key, self.key)
        assert_equal(self.seg.size, self.size)
        assert_equal(self.seg.name, self.seg_name)

    def test_02075_unlink_segment(self):
        """02075 rtapi shm:  unlink a segment"""

        self.seg2.unlink()
        assert_false(shmcommon.exists(self.key2))

    def test_02076_attach_unlinked_seg_fails(self):
        """02076 rtapi shm:  attach unlinked segment fails"""

        assert_raises(shmcommon.SHMCommonRuntimeError,
                      shmcommon.attach, self.key2)

    def test_02077_create_new_existing_seg_fails(self):
        """02077 rtapi shm:  create new existing segment fails"""

        assert_raises(shmcommon.SHMCommonRuntimeError,
                      shmcommon.new, self.key, self.size)

    def test_02078_unlink_last_segment(self):
        """02078 rtapi shm:  unlink last segment"""

        self.seg.unlink()
        assert_false(shmcommon.exists(self.key))
        assert_false(os.path.exists("/dev/shm/%s" % self.seg_name))
        assert_false(os.path.exists("/dev/shm/%s" % self.seg2_name))

