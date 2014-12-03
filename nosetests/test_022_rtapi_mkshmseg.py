from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi.config import Config, \
    RTAPIConfigException, RTAPIConfigNotFoundException
from machinekit.rtapi import MKSHMSegment, GlobalSegment, RTAPISegment, \
    HALSegment, SHMDrvAPIRuntimeError

import os, logging

@attr('config')
class test_022_rtapi_mkshmseg(FixtureTestCase):

    def test_02210_init_config(self):
        """02210 rtapi mkshmseg:  init config"""

        # add config fixtures
        self.fix(
            config = Config(enabled_stores=['test'],
                            store_config = {'test' : {'instance' : 0}}),
            )
        # add the 'test' store config dict for live manipulation
        self.fix(
            config_dict = self.config.stores.by_name('test').config_dict,
            )

    def test_02211_set_up_shm(self):
        """02211 rtapi mkshmseg:  set up shm"""

        # handy things for tests
        self.fix(
            classes = {
                # key magic is straight from rtapi_shmkeys.h
                'global' : dict(
                    cls = GlobalSegment,
                    magic = 0x00154711,
                    size = 531776, # sizeof(global_data_t)
                    ),
                'rtapi' : dict(
                    cls = RTAPISegment,
                    magic = 0x00280A48,
                    size = 10496, # rtapi_common.h, sizeof(rtapi_data_t)
                    ),
                'hal' : dict(
                    cls = HALSegment,
                    magic = 0x00414C32,
                    size = 262000, # config.h HAL_SIZE
                    ),
                },
            # List of instances to use in multi-instance tests; these
            # will be cleaned up before test runs
            test_instances = range(10) + [42, 127],
            )

        # set up shm w/instance, prefix, etc.
        MKSHMSegment.init_shm(self.config.instance)

        # quick sanity check
        assert_equal(self.config.instance,0)

        # clean up leftovers
        for key, segdata in self.classes.items():
            for instance in self.test_instances:
                print "Cleaning up '%s' key 0x%x, instance %d" % \
                    (key, segdata['magic'], instance)
                seg = MKSHMSegment(segdata['magic'], instance=instance)
                if seg.exists():
                    print "Unlinking existing key %s, instance %d" % \
                        (seg.posix_name, seg.instance)
                    seg.unlink()

    def test_02220_mk_seg_magic(self):
        """02220 rtapi mkshmseg:  mk shm segment magic"""
        for key, segdata in self.classes.items():
            seg = segdata['cls']()
            assert_equal(seg.magic, segdata['magic'])

    def test_02221_get_set_instance(self):
        """02221 rtapi mkshmseg:  get and set instance"""
        seg = GlobalSegment(instance = 0)
        
        # Increment class instance; seg instance should not change
        seg.instance = 0
        assert_equal(seg.instance, 0)
        for i in range(3):
            MKSHMSegment.instance = i
            print "instance: %d; class: %d; obj: %d" % \
                (i, MKSHMSegment.instance, seg.instance)
            assert_equal(MKSHMSegment.instance, i)
            assert_equal(GlobalSegment.instance, i)
            assert_equal(seg.instance, 0)

        # Increment seg instance; class instance should not change
        MKSHMSegment.instance = 0
        for i in range(3):
            seg.instance = i
            assert_equal(seg.instance, i)
            assert_equal(MKSHMSegment.instance, 0)

    def test_02222_shm_key_instances(self):
        """02222 rtapi mkshmseg:  shm keys for various instances"""

        for instance in self.test_instances:
            MKSHMSegment.instance = instance
            for key, segdata in self.classes.items():
                seg = segdata['cls']()
                assert_equal(seg.key, segdata['magic'] | (instance << 24))
        
        # check for exception
        MKSHMSegment.instance = 256 # first illegal number (8 MSBs of int)
        seg = GlobalSegment()
        assert_equal(GlobalSegment.instance, 256) # Sanity
        assert_not_equal(seg.key,
                         self.classes['global']['magic'] | \
                             (GlobalSegment.instance << 24))

        # reset instance
        MKSHMSegment.instance = 0

    def test_02223_shm_seg_posix_names(self):
        """02223 rtapi mkshmseg:  shm seg posix names"""
        
        for instance in self.test_instances:
            for key, segdata in self.classes.items():

                # Setup:  various segments w/various instances
                seg = segdata['cls'](instance=instance)
                assert_equal(seg.instance, instance) # Sanity

                # Test posix name
                assert_equal(
                    seg.posix_name,
                    '/linuxcnc-%08x' % (segdata['magic'] | (instance << 24)))

    def test_02224_mk_seg_sizes(self):
        """02224 rtapi mkshmseg:  mk shm segment sizes"""
        # this test will break when the global_data_t struct changes size

        for key, segdata in self.classes.items():

            # Setup:  segment w/default sizes
            seg = segdata['cls']()
            print "seg type = %s" % key
            print "seg.posix_name = %s" % seg.posix_name
            print "seg.requested_size = %d" % seg.requested_size
            print "seg.size = %d" % seg.size

            # Test:  segment sizes as expected
            assert_equal(seg.requested_size, segdata['size'])
            assert_equal(seg.size, segdata['size'])

    def test_02230_shm_seg_no_exists(self):
        """02230 rtapi mkshmseg:  shm segs do not exist"""

        for key, segdata in self.classes.items():

            # Setup:  various segment types
            seg = segdata['cls']()

            # Test:  segment doesn't exist
            assert_false(seg.exists())
        
    def test_02231_global_seg_create(self):
        """02231 rtapi mkshmseg:  create global seg"""

        # Setup:  create global seg as fixture
        self.fix(global_seg = GlobalSegment())
        self.global_seg.new()

        # Test:  global seg was created
        assert_true(self.global_seg.exists())

    def test_02232_global_seg_create(self):
        """02232 rtapi mkshmseg:  check global seg size"""

        # Setup:  global seg created
        assert_true(self.global_seg.exists())

        # Test:  expected size
        assert_equal(self.global_seg.size, self.classes['global']['size'])


    def test_02252_hal_rtapi_seg_sizes(self):
        """02252 rtapi mkshmseg:  HAL and RTAPI segment sizes"""

        # Setup:  create HAL seg
        hal_seg = HALSegment().new()
        assert_true(hal_seg.exists())

        # Test:  expected size
        assert_equal(hal_seg.size, self.classes['hal']['size'])

        # Cleanup
        hal_seg.unlink()
        assert_false(hal_seg.exists())
        
        # Setup:  create RTAPI seg
        rtapi_seg = RTAPISegment().new()
        assert_true(rtapi_seg.exists())

        # Test:  expected size
        assert_equal(rtapi_seg.size, self.classes['rtapi']['size'])

        # Cleanup
        rtapi_seg.unlink()
        assert_false(rtapi_seg.exists())
        

    def test_02253_instance_1_segs_create(self):
        """02253 rtapi mkshmseg:  create instance=1 global seg"""

        # Setup:  init inst 1 global seg object next to inst 0 global seg
        global_seg1 = GlobalSegment(instance=1)
        print ("global_seg1 name=%s, key=%08x, instance=%d, size=%d" % \
                   (global_seg1.name, global_seg1.key, global_seg1.instance,
                    global_seg1.size))
        assert_true(self.global_seg.exists())

        # Test:  inst1 seg does not exist
        assert_false(global_seg1.exists())

        # Setup:  create inst1 seg
        global_seg1.new()

        # Test:  inst1 seg exists next to inst0 seg
        assert_true(global_seg1.exists())
        assert_equal(global_seg1.instance, 1)
        assert_true(self.global_seg.exists())
        assert_equal(self.global_seg.instance, 0)

    def test_02254_segs_attach(self):
        """02254 rtapi mkshmseg:  attach to existings segs, instances 0 & 1"""

        # Setup:  new global seg
        seg0 = GlobalSegment()
        assert_true(seg0.exists())
        seg0.attach()

        # Setup:  attach to existing seg
        seg0_copy = GlobalSegment()
        assert_true(seg0_copy.exists())
        seg0_copy.attach()

        # Test:  segs point to same thing
        assert_equal(seg0.key, seg0_copy.key)
        assert_equal(seg0.posix_name, seg0_copy.posix_name)
        assert_equal(seg0.instance, 0)
        assert_equal(seg0_copy.instance, 0)

        # Setup:  attach to existing seg inst=1
        seg1 = GlobalSegment(instance=1)
        assert_true(seg1.exists())
        seg1.attach()

        # Test:  inst0 and inst1 segs not the same
        assert_not_equal(seg0.key, seg1.key)
        assert_equal(seg1.instance, 1)

    def test_02255_existing_seg_new_fails(self):
        """02255 rtapi mkshmseg:  creating existing seg fails"""

        # Setup:  inst0 global seg exists
        seg0 = GlobalSegment(instance=0)
        assert_true(seg0.exists())

        # Test:  new() inst0 global seg raises exception
        assert_raises(SHMDrvAPIRuntimeError, seg0.new)

        # Setup:  inst1 global seg exists
        MKSHMSegment.instance = 1
        seg1 = GlobalSegment(instance=1)
        assert_true(seg1.exists())

        # Test:  new() inst1 global seg raises exception
        assert_raises(SHMDrvAPIRuntimeError, seg1.new)
        

    def test_02256_unlink_segs(self):
        """02256 rtapi mkshmseg:  unlink some segs"""

        # Setup:  unlink segment
        seg0 = GlobalSegment(instance=0)
        seg0.unlink()

        # Test:  segment does not exist
        assert_false(seg0.exists())
        

    def test_02257_unlink_nonexistent_segs_fails(self):
        """02257 rtapi mkshmseg:  unlinking nonexistent segs fails"""

        # Setup:  create new segment and unlink
        seg = GlobalSegment(instance=0).new()
        assert_true(seg.exists())
        seg.unlink()
        assert_false(seg.exists())

        # Test:  second unlink() raises Exception
        assert_raises(SHMDrvAPIRuntimeError, seg.unlink)

        
    def test_02258_check_existing_segs_after_unlink(self):
        """02258 rtapi mkshmseg:  check existing segs after unlinking"""

        # Setup:  non-existent inst0 global seg
        seg0 = GlobalSegment(instance=0)
        assert_false(seg0.exists())

        # Setup:  inst1 global seg
        seg1 = GlobalSegment(instance=1)

        # Test:  still exists
        assert_true(seg1.exists())


    def test_02259_unlink_all_segs(self):
        """02259 rtapi mkshmseg:  unlink all segments"""

        # Setup:  inst1 global seg exists; inst0 global seg does not
        seg1 = GlobalSegment(instance=1)
        assert_true(seg1.exists())
        assert_false(GlobalSegment(instance=0).exists())

        # test:  unlink inst1 global seg
        seg1.unlink()
        assert_false(seg1.exists())


    def test_02260_exists_with_instance(self):
        """02260 rtapi mkshmseg:  exists() method with instance"""

        # Setup: default instance = 0, 'rtapi' segment obj
        MKSHMSegment.init_shm(instance=0)
        seg0 = RTAPISegment()
        seg1 = RTAPISegment(instance=1)

        # Test:  expected instance
        assert_equal(seg0.instance, 0)
        assert_equal(seg1.instance, 1)

        # Test:  insts 0, 1 all do not exist
        assert_false(seg0.exists())
        assert_false(seg1.exists())

        # Setup:  create 'rtapi' segment, instance 0
        seg0.new()

        # Test: inst0 exists, inst1 does not
        assert_true(seg0.exists())
        assert_false(seg1.exists())

        # Setup: default instance = 1, 'rtapi' segment obj
        MKSHMSegment.init_shm(instance=1)
        seg1 = RTAPISegment()

        # Test:  expected instance
        assert_equal(seg0.instance, 0)
        assert_equal(seg1.instance, 1)

        # Test: inst0 exists, inst1 does not
        assert_true(seg0.exists())
        assert_false(seg1.exists())

        # Setup:  create inst1 'rtapi' segment
        seg1.new()

        # Test: inst0 & 1 exist
        assert_true(seg0.exists())
        assert_true(seg1.exists())

        # Setup:  unlink inst0 'rtapi' segment
        seg0.unlink()

        # Test: inst0 does not exist, inst1 does
        assert_false(seg0.exists())
        assert_true(seg1.exists())

        # Setup:  unlink 'rtapi' segment, instance 1
        seg1.unlink()

        # Test: inst0 & 1 both do not exist
        assert_false(seg0.exists())
        assert_false(seg1.exists())

