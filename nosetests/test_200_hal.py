from . import RTAPITestCase
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit import hal

class test_200_hal(RTAPITestCase):


    cname = "pintest"
    pname = "out1"
    epsval = 10.0
    epsindex = 1
    signame = "ss32"
    pinval = 42

    @property
    def fqpname(self):
        return self.cname + "." + self.pname

    def test_20010_setup_epsindex(self):
        """20010 hal:  Setup component and pin"""

        hal.epsilon[self.epsindex] = self.epsval
        self.f.epsindexcomp = hal.Component(self.cname)

        self.f.epsindexcomp.newpin(self.pname, hal.HAL_S32, hal.HAL_OUT,
                                 init=self.pinval, eps=self.epsindex)
        self.f.epsindexcomp.ready()

    def test_20020_pin_creation_after_ready(self):
        """20020 hal:  Pin creation after ready() raises exception"""
        assert_raises(RuntimeError, self.f.epsindexcomp.newpin,
                      "in2", hal.HAL_S32, hal.HAL_IN)

    def test_20021_component_dictionary(self):
        """20021 hal:  Component exists in component dictionary"""
        assert_greater(len(hal.components()), 0)  # halcmd and others
        assert_in(self.cname, hal.components())
        assert_equal(hal.components[self.cname].name, self.cname)

    def test_20022_pins_dictionary(self):
        """20022 hal:  Pin exists in pins dictionary"""
        assert_equal(len(hal.pins), 1)
        assert_in(self.fqpname, hal.pins)
        assert_equal(hal.pins[self.fqpname].name, self.fqpname)

    def test_20023_pin_initial_value(self):
        """20023 hal:  Check pin current value equals initial value"""
        assert_equal(self.f.epsindexcomp[self.pname], self.pinval)

    def test_20024_pin_value_assignment(self):
        """20024 hal:  Set and check value"""
        self.f.epsindexcomp[self.pname] = 4711
        assert_equal(self.f.epsindexcomp[self.pname], 4711)

    def test_20025_pin_attributes(self):
        """20025 hal:  Check pin attributes"""
        n = self.f.epsindexcomp.pins()    # pin names of this comp
        assert_equal(len(n), 1)
        # access properties through wrapper:
        self.f.p = hal.Pin(n[0])
        assert_equal(self.f.p.name, self.fqpname)
        assert_equal(self.f.p.type, hal.HAL_S32)
        assert_equal(self.f.p.dir, hal.HAL_OUT)
        assert_equal(self.f.p.eps, self.epsindex)
        assert_almost_equal(self.f.p.epsilon, hal.epsilon[self.epsindex])
        assert_greater(self.f.p.handle, 0)
        assert_false(self.f.p.linked)

    def test_20030_signal_create(self):
        """20030 hal:  Create and check signal attributes"""
        self.f.s1 = hal.Signal(self.signame, hal.HAL_S32)
        assert_equal(self.f.s1.name, self.signame)
        assert_equal(self.f.s1.type, hal.HAL_S32)
        assert_equal(self.f.s1.readers, 0)
        assert_equal(self.f.s1.bidirs, 0)
        assert_equal(self.f.s1.writers, 0)
        assert_greater(self.f.s1.handle, 0)

    def test_20031_signal_set(self):
        """20031 hal:  Set and check signal value"""
        self.f.s1.set(12345)
        assert_equal(self.f.s1.get(), 12345)

    def test_20032_signals_dictionary(self):
        """20032 hal:  Check signal dictionary"""
        print "signals: %s" % hal.signals()
        assert_equal(len(hal.signals), 1)
        assert_in(self.signame, hal.signals)
        assert_equal(hal.signals[self.signame].name, self.signame)

    def test_20040_link(self):
        """20040 hal:  Link pin and signal"""
        assert_equal(self.f.s1.writers, 0)
        assert_is_none(self.f.s1.writername)

        self.f.s1.link(self.f.p)
        assert_equal(self.f.s1.writers, 1)
        assert_equal(self.f.s1.readers, 0)
        assert_equal(self.f.s1.bidirs, 0)

    def test_20041_link_attributes(self):
        """20041 hal:  Check link attributes"""
        # the list of Pin objects linked to this signal
        assert_equal(len(self.f.s1.pins()), 1)

        # the name of modifying pins linked to this signal
        assert_equal(self.f.s1.writername, self.fqpname)
        assert_is_none(self.f.s1.bidirname)

        # verify the pin reflects the signal it's linked to:
        assert_true(self.f.p.linked)
        assert_equal(self.f.p.signame, self.f.s1.name)
        sw = self.f.p.signal # access through Signal() wrapper
        assert_is_not_none(sw)
        assert_is_instance(sw,hal.Signal)
        assert_equal(self.f.p.signal.writers, 1)

        # initial value inheritage
        assert_equal(self.f.s1.get(), self.f.p.get())

    def test_20042_link_set_fail(self):
        """20042 hal:  Setting linked signal raises exception"""
        # since now linked, self.f.s1.set() must fail:
        assert_raises(RuntimeError,self.f.s1.set,271828)

    def test_20090_teardown_epsindex(self):
        """20090 hal:  Stop component"""
        hal.Signal.delete(self.signame)
        assert_equal(len(hal.signals()),0)
        self.f.epsindexcomp.exit()

