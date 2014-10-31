from . import FixtureTestCase
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi import Config, MKSHMSegment, SHMOps, GlobalData

import os, logging, resource

class test_052_rtapi_global(FixtureTestCase):

    def test_05210_init_config(self):
        """05210 rtapi global data:  init config"""

        self.fix(
            # a dict of test settings
            test_settings = dict(
                instance        = 0,
                mkuuid          = \
                    '22189ce8-2feb-443a-87e1-2214da0ef8bb',
                flavor          = 'posix',
                flavor_id       = 0,
                rtapi_msglevel  = 1,
                ulapi_msglevel  = 2,
                hal_size        = 262000,
                hal_stack_size  = 32768,
                ),
            magic = 0x0eadbeef,
            )

        self.fix(
            # add the config fixture
            config = Config(
                enabled_stores=['test'],
                store_config = dict(
                    test = self.test_settings,
                    ),
                ),
            )
        self.fix(
            # add the 'test' store config dict for live manipulation
            config_dict = self.config.stores.by_name('test').config_dict,
            shm = SHMOps(config=self.config),
            )

        # basically a translation between different names for the same
        # datum.  :P
        self.fix(
            attrdict = dict(
                magic                   = self.magic,
                instance_id             = self.config.instance,
                rtapi_thread_flavor     = self.config.flavor_id,
                rt_msg_level            = self.config.rtapi_msglevel,
                user_msg_level          = self.config.ulapi_msglevel,
                hal_size                = self.config.hal_size,
                hal_thread_stack_size   = self.config.hal_stack_size,
                service_uuid            = self.config.mkuuid,
                rtapi_app_pid           = -1,
                ),
            )

        # clean up any existing seg
        seg = MKSHMSegment('global')
        if seg.exists():
            seg.attach().unlink()

    def test_05211_open_global_shm_seg(self):
        """05211 rtapi global data:  open global shm segment"""

        self.fix(
            seg = self.shm.create_global_segment(),
            )

        # Sanity
        assert_true(MKSHMSegment.exists('global'))

    def test_05220_init_global_data(self):
        """05220 rtapi global data:  initialize global data"""

        # Create global data object and initialize data
        self.fix(
            gd = GlobalData(self.seg, self.config),
            )
        self.gd.init_global_data()

        # Check a basic attribute
        assert_equal(self.gd.magic, self.magic)

    def test_05221_check_attributes(self):
        """05221 rtapi global data:  check some attributes"""

        for (attr, value) in self.attrdict.items():
            print "attr %s:  global data=%s, want=%s" % \
                (attr, getattr(self.gd, attr), value)
            assert_equal(getattr(self.gd, attr), value)
