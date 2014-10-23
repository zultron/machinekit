from . import RTAPITestCase
from nose.tools import assert_not_in

from machinekit import rtapi,hal
import time

class test_280_hal_ring_demo(RTAPITestCase):

    def test_28010_loadrt_ringmods(self):
        """28010 hal ring demo:  loadrt"""
        self.rtapi.loadrt("ringload",   "num_rings=4", "size=16386")
        self.rtapi.loadrt("ringread",  "ring=ring_2")
        self.rtapi.loadrt("ringwrite", "ring=ring_2")
        self.rtapi.loadrt("charge_pump")

    def test_28020_net(self):
        """28020 hal ring demo:  Net pins"""
        hal.net("square-wave","charge-pump.out","ringwrite.write")

    def test_28030_start_threads(self):
        """28030 hal ring demo:  Start threads"""
        cpe = hal.Pin("charge-pump.enable")
        cpe.set(0)

        self.rtapi.newthread("fast",1000000, use_fp=True)
        self.rtapi.newthread("slow",100000000, use_fp=True)
        hal.addf("ringread","fast")
        hal.addf("ringwrite","slow")
        hal.addf("charge-pump","slow")
        hal.start_threads()
        cpe.set(1)    # enable charge_pump
        time.sleep(3) # let rt thread write a bit to ring

    def test_28040_stop_threads(self):
        """28040 hal ring demo:  Stop and delete threads"""
        hal.stop_threads()
        hal.delf("charge-pump","slow")
        hal.delf("ringwrite","slow")
        hal.delf("ringread","fast")
        self.rtapi.delthread("fast")
        self.rtapi.delthread("slow")

    def test_28090_cleanup(self):
        """28090 ring demo:  clean up"""
        self.rtapi.unloadrt("ringload")
        self.rtapi.unloadrt("ringread")
        self.rtapi.unloadrt("ringwrite")
        self.rtapi.unloadrt("charge_pump")

        hal.Ring.delete('ring_2')
        hal.Signal.delete('square-wave')
