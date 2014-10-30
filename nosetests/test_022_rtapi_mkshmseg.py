from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi.config import Config, \
    RTAPIConfigException, RTAPIConfigNotFoundException
from machinekit.rtapi import MKSHMSegment, SHMDrvAPIRuntimeError

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
            # key names
            keys = ('global', 'rtapi', 'hal'),
            # key magic, straight from rtapi_shmkeys.h
            magic = {
                'global' : 0x00154711,
                'hal' : 0x00414C32,
                'rtapi' : 0x00280A48,
                },
            seg_sizes = {
                'global' : 531792,
                'hal' : 262000,
                'rtapi' : 10496,
                },
            # List of instances to use in multi-instance tests; these
            # will be cleaned up before test runs
            test_instances = range(10) + [42, 127],
            )

        # FIXME testing
        self.config.log = self.log
        self.log.setLevel(logging.DEBUG)

        # set up shm w/instance, prefix, etc.
        MKSHMSegment.init_shm(self.config.instance)

        # quick sanity check
        assert_equal(self.config.instance,0)

        # clean up leftovers
        for key in self.keys:
            for instance in self.test_instances:
                seg = MKSHMSegment(key)
                seg.instance = instance
                if seg.exists():
                    self.log.warn("Unlinking existing segment %s" % key)
                    seg.attach().unlink()

    def test_02220_mk_seg_magic(self):
        """02220 rtapi mkshmseg:  mk shm segment magic"""
        for key in self.keys:
            seg = MKSHMSegment(key)
            assert_equal(seg.attr['magic'], self.magic[key])

    def test_02221_get_set_instance(self):
        """02221 rtapi mkshmseg:  get and set instance"""
        seg = MKSHMSegment('global')
        
        # Increment class instance; seg instance should not change
        seg.instance = 0
        assert_equal(seg.instance, 0)
        for i in range(3):
            MKSHMSegment._instance = i
            print "instance: %d; class: %d; obj: %d" % \
                (i, MKSHMSegment.instance, seg.instance)
            assert_equal(MKSHMSegment.instance, i)
            assert_equal(seg.instance, 0)

        # Increment seg instance; class instance should not change
        MKSHMSegment._instance = 0
        for i in range(3):
            seg.instance = i
            assert_equal(seg.instance, i)
            assert_equal(MKSHMSegment.instance, 0)

    def test_02222_shm_key_instances(self):
        """02222 rtapi mkshmseg:  shm keys for various instances"""

        for instance in self.test_instances:
            MKSHMSegment._instance = instance
            for key in self.keys:
                seg = MKSHMSegment(key)
                assert_equal(seg.key, self.magic[key] | (instance << 24))
        
        # check for exception
        MKSHMSegment._instance = 256 # first illegal number (8 MSBs of int)
        seg = MKSHMSegment('global')
        assert_not_equal(seg.key,
                         self.magic['global'] | (MKSHMSegment.instance << 24))

        # reset instance
        MKSHMSegment._instance = 0

    def test_02223_shm_seg_posix_names(self):
        """02223 rtapi mkshmseg:  shm seg posix names"""
        
        seg = MKSHMSegment('global')
        for instance in self.test_instances:
            MKSHMSegment._instance = instance
            for key in self.keys:
                seg = MKSHMSegment(key).new()
                assert_equal(seg.posix_name,
                             '/linuxcnc-%08x' %
                             ((self.magic[key] & 0x00ffffff) |
                              ((instance << 24) & 0xff000000)))
                seg.unlink()

    def test_02224_mk_seg_sizes(self):
        """02224 rtapi mkshmseg:  mk shm segment sizes"""
        # this will break when the global_data_t struct changes size
        seg = MKSHMSegment('global')
        assert_equal(seg.requested_size, self.seg_sizes['global'])

    def test_02230_shm_seg_no_exists(self):
        """02230 rtapi mkshmseg:  shm segs do not exist"""

        for key in self.keys:
            seg = MKSHMSegment(key)
            assert_false(seg.exists())
        
    def test_02231_global_seg_create(self):
        """02231 rtapi mkshmseg:  create global seg"""

        # create global seg as fixture
        self.fix(global_seg = MKSHMSegment('global'))
        self.global_seg.new()

        assert_true(self.global_seg.exists())

    def test_02232_global_seg_create(self):
        """02232 rtapi mkshmseg:  check global seg size"""

        assert_equal(self.global_seg.size, self.seg_sizes['global'])


    def test_02233_exists_object_class_methods(self):
        """02233 rtapi mkshmseg:  exists() object and class methods"""

        assert_true(self.global_seg.exists())
        assert_true(MKSHMSegment.exists('global'))

        assert_false(MKSHMSegment('hal').exists())
        assert_false(MKSHMSegment.exists('hal'))


    def test_02252_hal_rtapi_seg_sizes(self):
        """02252 rtapi mkshmseg:  HAL and RTAPI segment sizes"""

        # create hal seg & check size
        hal_seg = MKSHMSegment('hal').new()
        assert_true(hal_seg.exists())
        assert_equal(hal_seg.size, self.seg_sizes['hal'])
        hal_seg.unlink()
        assert_false(hal_seg.exists())
        
        # create rtapi seg & check size
        rtapi_seg = MKSHMSegment('rtapi').new()
        assert_true(rtapi_seg.exists())
        assert_equal(rtapi_seg.size, self.seg_sizes['rtapi'])
        rtapi_seg.unlink()
        assert_false(rtapi_seg.exists())
        

    def test_02253_instance_1_segs_create(self):
        """02253 rtapi mkshmseg:  create instance=1 global seg"""

        MKSHMSegment._instance = 1
        global_seg1 = MKSHMSegment('global')
        print "global_seg1 name=%s, key=%08x, instance=%d, size=%d" % \
            (global_seg1.name, global_seg1.key, global_seg1.instance,
             global_seg1.size)
        assert_false(global_seg1.exists())
        global_seg1.new()
        assert_true(global_seg1.exists())

        MKSHMSegment._instance = 0

    def test_02254_segs_attach(self):
        """02254 rtapi mkshmseg:  attach to existings segs, instances 0 & 1"""

        MKSHMSegment._instance = 0
        seg0 = MKSHMSegment('global')
        assert_false(seg0.exists())
        seg0.new()

        seg0_copy = MKSHMSegment('global')
        assert_true(seg0_copy.exists())
        seg0_copy.attach()

        assert_equal(seg0.key, seg0_copy.key)

        MKSHMSegment._instance = 1
        seg1 = MKSHMSegment('global')
        assert_true(seg1.exists())
        seg1.attach()
        assert_not_equal(seg0.key, seg1.key)

        MKSHMSegment._instance = 0

    def test_02255_existing_seg_new_fails(self):
        """02255 rtapi mkshmseg:  creating existing seg fails"""

        MKSHMSegment._instance = 0
        seg0 = MKSHMSegment('global')
        assert_true(seg0.exists())
        assert_raises(SHMDrvAPIRuntimeError, seg0.new)

        MKSHMSegment._instance = 1
        seg1 = MKSHMSegment('global')
        assert_true(seg1.exists())
        assert_raises(SHMDrvAPIRuntimeError, seg1.new)

        MKSHMSegment._instance = 0
        

    def test_02256_unlink_segs(self):
        """02256 rtapi mkshmseg:  unlink some segs"""

        MKSHMSegment._instance = 0
        seg0 = MKSHMSegment('global').attach()
        seg0.unlink()
        assert_false(seg0.exists())
        

    def test_02257_unlink_nonexistent_segs_fails(self):
        """02257 rtapi mkshmseg:  unlinking nonexistent segs fails"""

        MKSHMSegment._instance = 0
        seg = MKSHMSegment('global').new()
        seg.unlink()
        assert_raises(SHMDrvAPIRuntimeError, seg.unlink)

        
    def test_02258_check_existing_segs_after_unlink(self):
        """02258 rtapi mkshmseg:  check existing segs after unlinking"""

        # prereq:  seg0 unlinked
        MKSHMSegment._instance = 0
        seg0 = MKSHMSegment('global')
        assert_false(seg0.exists())

        # test:  seg1 still exists
        MKSHMSegment._instance = 1
        seg1 = MKSHMSegment('global')
        assert_true(seg1.exists())
        seg1.attach()
        assert_equal(seg1.name,'global')

        # reset
        MKSHMSegment._instance = 0


    def test_02259_unlink_all_segs(self):
        """02259 rtapi mkshmseg:  unlink all segments"""

        # prereq:  seg1 exists
        MKSHMSegment._instance = 1
        seg1 = MKSHMSegment('global')
        assert_true(seg1.exists())

        # test:  unlink seg1
        seg1.attach().unlink()
        assert_false(seg1.exists())

        # reset
        MKSHMSegment._instance = 0


    def test_02260_exists_with_instance(self):
        """02260 rtapi mkshmseg:  exists() method with instance"""

        name = 'rtapi'

        # Setup: set default instance to 0, init 'rtapi' segment
        # object seg0
        MKSHMSegment.init_shm(instance=0)
        seg0 = MKSHMSegment(name)

        # Test:  all forms of exists() method return False, insts 0, 1
        assert_false(seg0.exists())
        # the following three should be equivalent
        assert_false(MKSHMSegment.exists(name))
        assert_false(MKSHMSegment.exists(name,0))
        assert_false(MKSHMSegment.exists(name,instance=0))
        # the following two should be equivalent
        assert_false(MKSHMSegment.exists(name,1))
        assert_false(MKSHMSegment.exists(name,instance=1))

        # Setup:  create 'rtapi' segment, instance 0
        seg0.new()

        # Test: all forms of exists() method return True inst 0, False
        # inst 1
        assert_true(seg0.exists())
        # the following three should be equivalent
        assert_true(MKSHMSegment.exists(name))
        assert_true(MKSHMSegment.exists(name,0))
        assert_true(MKSHMSegment.exists(name,instance=0))
        # the following two should be equivalent
        assert_false(MKSHMSegment.exists(name,1))
        assert_false(MKSHMSegment.exists(name,instance=1))

        # Setup: set default instance to 1, init 'rtapi' segment
        # object seg1
        MKSHMSegment.init_shm(instance=1)
        seg1 = MKSHMSegment(name)

        # Test: all forms of exists() method return True inst 0, False
        # inst 1
        assert_true(seg0.exists())
        assert_false(seg1.exists())
        # the following two should be equivalent
        assert_true(MKSHMSegment.exists(name,0))
        assert_true(MKSHMSegment.exists(name,instance=0))
        # the following three should be equivalent
        assert_false(MKSHMSegment.exists(name))
        assert_false(MKSHMSegment.exists(name,1))
        assert_false(MKSHMSegment.exists(name,instance=1))

        # Setup:  create 'rtapi' segment, instance 1
        seg1.new()

        # Test: all forms of exists() method return True insts 0, 1
        assert_true(seg0.exists())
        assert_true(seg1.exists())
        # the following two should be equivalent
        assert_true(MKSHMSegment.exists(name,0))
        assert_true(MKSHMSegment.exists(name,instance=0))
        # the following three should be equivalent
        assert_true(MKSHMSegment.exists(name))
        assert_true(MKSHMSegment.exists(name,1))
        assert_true(MKSHMSegment.exists(name,instance=1))

        # Setup:  unlink 'rtapi' segment, instance 0
        seg0.unlink()

        # Test: all forms of exists() method return False inst 0, True
        # inst 1
        assert_false(seg0.exists())
        assert_true(seg1.exists())
        # the following three should be equivalent
        assert_false(MKSHMSegment.exists(name,0))
        assert_false(MKSHMSegment.exists(name,instance=0))
        # the following two should be equivalent
        assert_true(MKSHMSegment.exists(name))
        assert_true(MKSHMSegment.exists(name,1))
        assert_true(MKSHMSegment.exists(name,instance=1))

        # Setup:  unlink 'rtapi' segment, instance 1
        seg1.unlink()

        # Test: all forms of exists() method return False inst 0, True
        # inst 1
        assert_false(seg0.exists())
        assert_false(seg1.exists())
        # the following three should be equivalent
        assert_false(MKSHMSegment.exists(name,0))
        assert_false(MKSHMSegment.exists(name,instance=0))
        # the following two should be equivalent
        assert_false(MKSHMSegment.exists(name))
        assert_false(MKSHMSegment.exists(name,1))
        assert_false(MKSHMSegment.exists(name,instance=1))

