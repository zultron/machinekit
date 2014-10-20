from . import RTAPITestCase, RunWithLog, Run
from nose.tools import assert_equal, assert_is_not_none

from machinekit import hal
import time

class test_000_rtapi(RTAPITestCase):

    env = {
        "DEBUG" : "5",
        "RTAPI_FOREGROUND" : "1",
    }

    __test__ = True

    def test_00001_start_rt(self):
        """00001 rtapi:  Start realtime environment"""
        RunWithLog(["realtime","restart"], "nosetests.rt.log", self.env).start()
        assert_equal(Run.simple(["halcmd","ping"]),0)

    def test_00010_rtapi_connect(self):
        """00010 rtapi: Connect to RTAPI"""
        # This really just makes sure RTAPITestCase.rtapi has been
        # initialized; it should be the very first test in the module
        assert_is_not_none(self.rtapi)

    def test_00020_loadrt_or2(self):
        """00020 rtapi: Load a component and start threads"""
        self.rtapi.loadrt("or2")
        self.rtapi.newthread("servo-thread",1000000,use_fp=True)
        hal.addf("or2.0","servo-thread")
        hal.start_threads()
        time.sleep(0.2)

    def test_00030_unloadrt_or2(self):
        """00030 rtapi: Stop threads and unload component"""
        hal.stop_threads()
        self.rtapi.delthread("servo-thread")
        self.rtapi.unloadrt("or2")
