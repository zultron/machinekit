#!/usr/bin/env python

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_raises, \
    assert_equal, assert_almost_equal, assert_not_in, assert_in, \
    assert_true, assert_false

from machinekit import hal

@test(groups=["hal"],
      depends_on_groups=["hal_pins"])
class TestNetCmd(RTAPITestCase):

    @before_class
    def init_pins(self):
        """Net:  Initialize components and pins"""
        c1 = hal.Component("c1")
        c1.newpin("s32out", hal.HAL_S32, hal.HAL_OUT, init=42)
        c1.newpin("s32in",  hal.HAL_S32, hal.HAL_IN)
        c1.newpin("s32io",  hal.HAL_S32, hal.HAL_IO)
        c1.newpin("floatout", hal.HAL_FLOAT, hal.HAL_OUT, init=42)
        c1.newpin("floatin",  hal.HAL_FLOAT, hal.HAL_IN)
        c1.newpin("floatio",  hal.HAL_FLOAT, hal.HAL_IO)
        c1.ready()
        self.c1 = c1

        c2 = hal.Component("c2")
        c2.newpin("s32out", hal.HAL_S32, hal.HAL_OUT, init=4711)
        c2.newpin("s32in",  hal.HAL_S32, hal.HAL_IN)
        c2.newpin("s32io",  hal.HAL_S32, hal.HAL_IO)
        c2.newpin("floatout", hal.HAL_FLOAT, hal.HAL_OUT, init=4711)
        c2.newpin("floatin",  hal.HAL_FLOAT, hal.HAL_IN)
        c2.newpin("floatio",  hal.HAL_FLOAT, hal.HAL_IO)
        c2.ready()
        self.c2 = c2

        # should have six pins defined in each component
        assert_equal(len(c1.pins()),6)
        assert_equal(len(c2.pins()),6)

    @test
    def net_existing_signal_with_bad_type(self):
        """Net:  Net existing signal with wrong type raises exception"""
        hal.new_sig("f", hal.HAL_FLOAT)
        assert_raises(TypeError, hal.net, "f", "c1.s32out")
        hal.Signal.delete("f")

    @test
    def net_match_nonexistant_signals(self):
        """Net:  Net nonexistent signal raises exception"""
        assert_raises(TypeError, hal.net, "nosuchsig", "c1.s32out","c2.s32out")

    @test
    def net_pin2pin(self):
        """Net:  Net pin to pin raises exception"""
        assert_raises(TypeError, hal.net, "c1.s32out","c2.s32out")
        #TypeError: net: 'c1.s32out' is a pin - first argument
        #must be a signal name

    @test
    def net_existing_signal(self):
        """Net:  Net existing signal raises exception"""
        hal.new_sig("s32", hal.HAL_S32)

        assert_false(hal.pins["c1.s32out"].linked)
        hal.net("s32", "c1.s32out")
        assert_true(hal.pins["c1.s32out"].linked)

        hal.new_sig("s32too", hal.HAL_S32)
        assert_raises(RuntimeError, hal.net, "s32too", "c1.s32out")

        hal.Signal.delete("s32")

    @test
    def new_sig(self):
        """Net:  Net new signal"""
        floatsig1 = hal.new_sig("floatsig1", hal.HAL_FLOAT)
        assert_raises(RuntimeError, hal.new_sig, "floatsig1", hal.HAL_FLOAT)
        assert_raises(TypeError, hal.new_sig, 32423 *32432, hal.HAL_FLOAT)
        assert_raises(TypeError, hal.new_sig, None, hal.HAL_FLOAT)
        assert_raises(TypeError, hal.new_sig, "badtype", 1234)

    @test
    def net_args_incorrect(self):
        """Net:  Incorrect net arguments raise exceptions"""
        assert_raises(TypeError, hal.net)
        assert_raises(TypeError, hal.net, None, "c1.s32out")
        assert_raises(TypeError, hal.net, "c1.s32out")

    @test
    def net_dict(self):
        """Net:  Net dictionary"""
        assert_not_in("noexiste", hal.signals)
        hal.net("noexiste", "c1.s32out")
        assert_in("noexiste", hal.signals)
        hal.Signal.delete("noexiste")
        assert_not_in("noexiste", hal.signals(),
                      "Signal 'noexiste' still exists after delete: %s" % \
                          hal.signals())

    @test(depends_on=[net_dict])
    def net_attributes(self):
        """Net:  Net attribute checks"""
        hal.net("asig", "c1.s32out")
        s = hal.signals["asig"]

        assert_equal(s.writers, 1)
        assert_equal(s.readers, 0)
        assert_equal(s.bidirs, 0)

        hal.Signal.delete("asig")

    @test
    def net_type_mismatch(self):
        """Net:  Signal and pin type mismatch raises exception"""
        hal.new_sig("floatsig2", hal.HAL_FLOAT)
        assert_raises(TypeError, hal.net, "floatsig2", "c2.s32out")

    @after_class
    def cleanup(self):
        """Net:  Clean up"""
        hal.Signal.delete("floatsig1")
        hal.Signal.delete("floatsig2")
        hal.Signal.delete("s32too")

        self.c1.exit()
        self.c2.exit()

        check_hal_clean()
