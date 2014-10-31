from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi import GlobalData, RTAPIGlobalDataException, \
    Config, MKSHMSegment

import os, logging, resource

class test_050_rtapi_global(FixtureTestCase):

    def test_05010_init_config(self):
        """05010 rtapi global:  init config"""

        # add config fixtures
        self.fix(
            config = Config(
                enabled_stores=['test'],
                store_config = {
                    'test' : {
                        'instance' : 0,
                        'mkuuid' : '22189ce8-2feb-443a-87e1-2214da0ef8bb',
                        'flavor' : 2,
                        },
                    },
                ),
            )
        # add the 'test' store config dict for live manipulation
        self.fix(config_dict = self.config.stores.by_name('test').config_dict)

        # clean up any existing seg
        seg = MKSHMSegment('global')
        if seg.exists():
            seg.attach().unlink()

    def test_05011_set_up_global_shm_seg(self):
        """05011 rtapi global:  set up global shm segment"""

        # handy things for tests
        self.fix(
            # key magic, straight from rtapi_shmkeys.h
            magic = 0x00154711,
            size = 531792,
            seg = MKSHMSegment('global'),
            )

        assert_true(self.seg.new().exists())

    def test_05012_global_shm_seg_size(self):
        """05012 rtapi global:  global shm segment size"""

        assert_equal(self.seg.size, self.size)

    def test_05020_init_global_data_obj(self):
        """05020 rtapi global:  init global data object"""

        # Setup:  initialize gd shm segment
        self.fix(
            gd = GlobalData(self.seg),
            )

        # Test:  gd object pointer == shm seg ptr
        assert_equal(self.gd.ptr, self.seg.ptr)

    def test_05030_attribute_read_write(self):
        """05030 rtapi global:  attribute read/write"""

        # Setup: list of attribute : (value, zero) pairs
        self.fix(
            attrdict = dict(
                magic = (self.magic, 0),
                instance_id = (self.config.instance, 0),
                rtapi_thread_flavor = (self.config.flavor, 0),
                rt_msg_level = (5, 0),
                user_msg_level = (4, 0),
                hal_size = (self.config.hal_size, 0),
                #hal_thread_stack_size = ???,
                service_uuid = (self.config.mkuuid,
                                '00000000-0000-0000-0000-000000000000'),
                rtapi_app_pid = (42, 0),
                rtapi_msgd_pid = (13, 0),
                ),
            )

        # Test:  write attributes and verify read
        for (attr, value_zero) in self.attrdict.items():
            value, zero = value_zero
            setattr(self.gd, attr, value)
            assert_equal(getattr(self.gd, attr), value)


    def test_05031_reattach_attribute_read(self):
        """05031 rtapi global:  reattach and read attribute"""

        # Setup:  attach new object to existing data segment
        seg_copy = MKSHMSegment('global').attach()
        gd_copy = GlobalData(seg_copy)

        # Test:  write attributes and verify read
        for (attr, value_zero) in self.attrdict.items():
            value, zero = value_zero
            assert_equal(getattr(self.gd, attr), value)
            assert_equal(getattr(self.gd, attr), getattr(gd_copy, attr))

    def test_05040_zero(self):
        """05040 rtapi global:  zero"""

        # Test: zero global seg and test some values
        self.gd.zero()
        for (attr, value_zero) in self.attrdict.items():
            value, zero = value_zero
            assert_equal(getattr(self.gd, attr), zero)

    def test_05041_mlock_munlock(self):
        """05041 rtapi global:  mlock and munlock"""

        # Read memlock rlimits
        (memlock_rlimit_soft, memlock_rlimit_hard) = \
            resource.getrlimit(resource.RLIMIT_MEMLOCK)
        print "memlock rlimit %d; seg size %d" % \
            (memlock_rlimit_soft, self.seg.size)

        # Sanity check:  rlimit high enough
        assert_greater(memlock_rlimit_soft, self.seg.size)

        # Test:  mlock global seg; no exception
        self.gd.mlock()

        # Test:  munlock global seg:  no exception
        self.gd.munlock()

        # Setup:  set memlock rlimit smaller than segment size
        resource.setrlimit(resource.RLIMIT_MEMLOCK,
                           (self.seg.size-10, memlock_rlimit_hard))

        # Test:  mlock global seg exception
        assert_raises(RTAPIGlobalDataException,
                      self.gd.mlock)

        # Setup: reset memlock rlimit
        resource.setrlimit(resource.RLIMIT_MEMLOCK,
                           (memlock_rlimit_soft, memlock_rlimit_hard))

        # Test:  mlock global seg; no exception
        self.gd.mlock()

    def test_05045_mutex(self):
        """05045 rtapi global:  mutex try and give functions"""

        # Sanity:  mutex is 0
        assert_equal(self.gd.mutex, 0)

        # Test:  obtain mutex; no exception
        self.gd.mutex_try()

        # Test:  fail to obtain mutex
        assert_raises(RTAPIGlobalDataException,
                      self.gd.mutex_try)

        # Test:  release mutex; no exception
        self.gd.mutex_give()

        # Test:  obtain mutex after release; no exception
        self.gd.mutex_try()

        # Reset:  release mutex
        self.gd.mutex_give()

    @skip("Unwritten")
    def test_05050_error_ring_init(self):
        """05050 rtapi global:  error_ring_init()"""
        pass

    @skip("Unwritten")
    def test_05055_init_rtapi_heap(self):
        """05055 rtapi global:  init_rtapi_heap()"""
        pass

