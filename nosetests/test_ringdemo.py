#!/usr/bin/env python

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_not_in

from machinekit import rtapi,hal
import time

@test(groups=["hal"],
      depends_on_groups=["hal_ring_rw","rtapi_base"])
class RingDemo(RTAPITestCase):

    @before_class
    def check_environment(self):
        """Ring Demo:  Environment is clean"""
        # check the ring doesn't already exist for some reason
        assert_not_in("ring1",hal.rings(),
                      "Ring 'ring1' already exists; rings: %s" % hal.rings())

    @test
    def loadrt_ringmods(self):
        """Ring Demo:  loadrt"""
        self.rtapi.loadrt("ringload",   "num_rings=4", "size=16386")
        self.rtapi.loadrt("ringread",  "ring=ring_2")
        self.rtapi.loadrt("ringwrite", "ring=ring_2")
        self.rtapi.loadrt("charge_pump")

    @test(depends_on=[loadrt_ringmods])
    def net(self):
        """Ring Demo:  Net pins"""
        hal.net("square-wave","charge-pump.out","ringwrite.write")

    @test(depends_on=[net])
    def runthread(self):
        """Ring Demo:  Start threads"""
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

    @test(depends_on=[runthread])
    def stopthread(self):
        """Ring Demo:  Stop and delete threads"""
        hal.stop_threads()
        hal.delf("charge-pump","slow")
        hal.delf("ringwrite","slow")
        hal.delf("ringread","fast")
        self.rtapi.delthread("fast")
        self.rtapi.delthread("slow")

    @after_class
    def cleanup(self):
        """Ring Demo:  clean up"""
        self.rtapi.unloadrt("ringload")
        self.rtapi.unloadrt("ringread")
        self.rtapi.unloadrt("ringwrite")
        self.rtapi.unloadrt("charge_pump")

        hal.Ring.delete('ring_2')
        hal.Signal.delete('square-wave')

        check_hal_clean()

