from utils import RTAPITestCase, check_hal_clean
from proboscis import test, after_class
from proboscis.asserts import assert_is_not_none

from machinekit import hal
import time

@test(groups=["rtapi","rtapi_base"])
class TestRtapiBase(RTAPITestCase):

    @test
    def rtapi_connect(self):
        """RtapiBase: Connect to RTAPI"""
        # This really just makes sure RTAPITestCase.rtapi has been
        # initialized; it should be the first test
        r = self.rtapi
        assert_is_not_none(r)

    @test(depends_on=[rtapi_connect])
    def loadrt_or2(self):
        """RtapiBase: Load a component and start threads"""
        self.rtapi.loadrt("or2")
        self.rtapi.newthread("servo-thread",1000000,use_fp=True)
        hal.addf("or2.0","servo-thread")
        hal.start_threads()
        time.sleep(0.2)

    @test(depends_on=[loadrt_or2])
    def unloadrt_or2(self):
        """RtapiBase: Stop threads and unload component"""
        hal.stop_threads()
        self.rtapi.delthread("servo-thread")
        self.rtapi.unloadrt("or2")

    @after_class
    def check_clean(self):
        """RtapiBase: Check HAL is clean"""
        check_hal_clean()
