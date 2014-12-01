from . import RTAPITestCase, RunWithLog, Run
from nose.tools import assert_equal, assert_is_none, assert_is_not_none

from machinekit.rtapi import Config, Util

import os

class test_120_rtapi_util(RTAPITestCase):

    def test_12010_setup(self):
        """12010 rtapi util:  setup"""
        
        self.add_fixture(
            'config',
            Config(
                enabled_stores=['test', 'flavor', 'current_flavor'],
                store_config = {'test' : {'flavor' : 'posix',
                                          'instance' : 0}}))
        self.add_fixture('util', Util(self.config))


    def test_12020_proc_by_cmd_msgd(self):
        """12020 rtapi util:  util.proc_by_cmd(msgd:0)"""
        proc = self.util.proc_by_cmd("msgd:%d" % self.config.instance)
        assert_is_not_none(proc,"Check if realtime is running")
        assert_equal(proc.name, 'rtapi_msgd')

    def test_12021_proc_by_name_rtapi(self):
        """12020 rtapi util:  util.proc_by_name(rtapi:0)"""
        proc = self.util.proc_by_name("rtapi:%d" % self.config.instance)
        assert_is_not_none(proc,"Check if realtime is running")
        assert_equal(proc.cmdline[0], 'rtapi:%d' % self.config.instance)

