from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi.config import Config, \
    RTAPIConfigException, RTAPIConfigNotFoundException
from machinekit.rtapi import \
    SHMOps, RTAPISHMRuntimeError, \
    MKSHMSegment, GlobalSegment, RTAPISegment, HALSegment, SHMDrvAPIRuntimeError

import os, logging

@attr('config')
class test_024_rtapi_shmops(FixtureTestCase):

    def test_02410_init_config(self):
        """02410 rtapi shmops:  init config"""

        # add config fixtures
        self.fix(
            config = Config(enabled_stores=['test'],
                            store_config = {'test' : {'instance' : 0}}),
            )

        # add the 'test' store config dict for live manipulation
        self.fix(
            config_dict = self.config.stores.by_name('test').config_dict,
            )

        # quick sanity check
        assert_equal(self.config.instance,0)


    def test_02411_init_shmops(self):
        """02411 rtapi shmops:  init shmops"""

        # shmops fixture
        self.fix(
            shmops = SHMOps(config=self.config),
            )

        # init shmdrv_api
        self.shmops.init_shm()


    @skip("Not written; depends on tests 0208*")
    def test_02420_shmdrv_available(self):
        """02420 rtapi shmops:  shmdrv_available()"""
        pass


    @skip("Not written; depends on tests 0208*")
    def test_02421_init_shmdrv(self):
        """02421 rtapi shmops:  init_shmdrv()"""
        pass


    @skip("Not written; depends on tests 0208*")
    def test_02422_cleanup_shmdrv(self):
        """02422 rtapi shmops:  cleanup_shmdrv()"""
        pass


    def test_02430_any_segment_exists(self):
        """02430 rtapi shmops:  any_segment_exists()"""

        # Sanity check on list of segments
        for cls in (GlobalSegment, RTAPISegment, HALSegment):
            assert_in(cls, self.shmops.all_seg_classes)

        # Setup:  no segment exists
        for cls in self.shmops.all_seg_classes:
            assert_false(cls().exists())

        # Test:  any_segment_exists() returns False
        assert_false(self.shmops.any_segment_exists())

        for cls in self.shmops.all_seg_classes:
            # Setup:  create segment
            seg = cls().new()

            # Test:  any_segment_exists() is True
            assert_true(self.shmops.any_segment_exists())

            # Setup:  remove segment
            seg.unlink()

            # Test:  any_segment_exists() is False
            assert_false(self.shmops.any_segment_exists())

    def test_02440_cleanup_shm_posix(self):
        """02440 rtapi shmops:  cleanup_shm_posix()"""

        # Setup:  no segs exist
        for cls in self.shmops.all_seg_classes:
            assert_false(cls().exists())

        # Test:  no exception when barf=False
        self.shmops.cleanup_shm_posix(barf=False)

        # Test:  exception when barf=True
        assert_raises(RTAPISHMRuntimeError,
                      self.shmops.cleanup_shm_posix,barf=True)

        # Setup:  'hal' and 'global' segs exist, but 'rtapi' missing
        HALSegment().new()
        GlobalSegment().new()

        # Test:  exception when barf=True
        assert_raises(RTAPISHMRuntimeError,
                      self.shmops.cleanup_shm_posix,barf=True)

        # Test:  global seg still exists (rtapi cleaned up; barfed on hal)
        assert_true(GlobalSegment().exists())
        assert_false(HALSegment().exists())
        assert_false(RTAPISegment().exists())

        # Test:  no exception when barf=False
        self.shmops.cleanup_shm_posix(barf=False)

        # Test:  all segs unlinked
        for cls in self.shmops.all_seg_classes:
            assert_false(cls().exists())


    def test_02450_assert_segment_sanity_posix(self):
        """02450 rtapi shmops:  assert_segment_sanity() for POSIX shm"""
        
        # Setup:  no segment exists
        for cls in self.shmops.all_seg_classes:
            assert_false(cls().exists())

        # Test:  no exception
        self.shmops.assert_segment_sanity()

        # Setup:  a segment exists; use_shmdrv is False
        seg = GlobalSegment().new()
        self.config_dict['use_shmdrv'] = False

        # Test: no exception
        self.shmops.assert_segment_sanity()

        # Setup:  simulate failure to clean up all segs
        seg = GlobalSegment().new()
        # give shmops object a no-op cleanup_shm_posix function
        self.shmops.cleanup_shm_posix = lambda barf: None;

        # Test: exception
        assert_raises(RTAPISHMRuntimeError,
                      self.shmops.assert_segment_sanity)

        # Reset shmops object & test
        del self.shmops.cleanup_shm_posix
        self.shmops.assert_segment_sanity()


    @skip("Unwritten")
    def test_02451_assert_segment_sanity_shmdrv(self):
        """02451 rtapi shmops:  assert_segment_sanity() for shmdrv shm"""
        pass


    @skip("Unwritten")
    def test_02460_assert_sanity(self):
        """02460 rtapi shmops:  assert_sanity()"""
        pass


    def test_02470_create_global_segment(self):
        """02470 rtapi shmops:  create_global_segment()"""

        # Setup:  no global seg exists
        assert_false(GlobalSegment().exists())

        # Test:  global seg exists after execution
        self.shmops.create_global_segment()
        assert_true(GlobalSegment().exists())
