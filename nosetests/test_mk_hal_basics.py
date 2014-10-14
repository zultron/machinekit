#!/usr/bin/env python

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from proboscis.asserts import assert_is_not_none
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit import hal
import os

@test(groups=["hal", "hal_base"])
class TestEpsindex(RTAPITestCase):

    @before_class
    def setup_epsindex(self):
        """Epsindex:  Setup component and pin"""
        self.cname = "pintest%d" % os.getpid()
        self.pname = "out1"
        self.epsval = 10.0
        self.epsindex = 1
        self.signame = "ss32"
        self.fqpname = self.cname + "." + self.pname
        self.pinval = 42

        hal.epsilon[self.epsindex] = self.epsval
        self.epsindexcomp = hal.Component(self.cname)

        self.epsindexcomp.newpin(self.pname, hal.HAL_S32, hal.HAL_OUT,
                                 init=self.pinval, eps=self.epsindex)
        self.epsindexcomp.ready()

    @test
    def pin_creation_after_ready(self):
        """Epsindex:  Pin creation after ready() raises exception"""
        assert_raises(RuntimeError, self.epsindexcomp.newpin,
                      "in2", hal.HAL_S32, hal.HAL_IN)

    @test
    def component_dictionary(self):
        """Epsindex:  Component exists in component dictionary"""
        assert_greater(len(hal.components()), 0)  # halcmd and others
        assert_in(self.cname, hal.components())
        assert_equal(hal.components[self.cname].name, self.cname)

    @test
    def pins_dictionary(self):
        """Epsindex:  Pin exists in pins dictionary"""
        assert_equal(len(hal.pins), 1)
        assert_in(self.fqpname, hal.pins)
        assert_equal(hal.pins[self.fqpname].name, self.fqpname)

    @test
    def pin_initial_value(self):
        """Epsindex:  Check pin current value equals initial value"""
        assert_equal(self.epsindexcomp[self.pname], self.pinval)

    @test
    def pin_value_assignment(self):
        """Epsindex:  Set and check value"""
        self.epsindexcomp[self.pname] = 4711
        assert_equal(self.epsindexcomp[self.pname], 4711)

    @test
    def pin_attributes(self):
        """Epsindex:  Check pin attributes"""
        n = self.epsindexcomp.pins()    # pin names of this comp
        assert_equal(len(n), 1)
        # access properties through wrapper:
        self.p = hal.Pin(n[0])
        assert_equal(self.p.name, self.fqpname)
        assert_equal(self.p.type, hal.HAL_S32)
        assert_equal(self.p.dir, hal.HAL_OUT)
        assert_equal(self.p.eps, self.epsindex)
        assert_almost_equal(self.p.epsilon, hal.epsilon[self.epsindex])
        assert_greater(self.p.handle, 0)
        assert_false(self.p.linked)

    @test
    def signal_create(self):
        """Epsindex:  Create and check signal attributes"""
        self.s1 = hal.Signal(self.signame, hal.HAL_S32)
        assert_equal(self.s1.name, self.signame)
        assert_equal(self.s1.type, hal.HAL_S32)
        assert_equal(self.s1.readers, 0)
        assert_equal(self.s1.bidirs, 0)
        assert_equal(self.s1.writers, 0)
        assert_greater(self.s1.handle, 0)

    @test(depends_on=[signal_create])
    def signal_set(self):
        """Epsindex:  Set and check signal value"""
        self.s1.set(12345)
        assert_equal(self.s1.get(), 12345)

    @test(depends_on=[signal_set])
    def link(self):
        """Epsindex:  Link pin and signal"""
        assert_equal(self.s1.writers, 0)
        assert_is_none(self.s1.writername)

        self.s1.link(self.p)
        assert_equal(self.s1.writers, 1)
        assert_equal(self.s1.readers, 0)
        assert_equal(self.s1.bidirs, 0)

    @test(depends_on=[link])
    def link_attributes(self):
        """Epsindex:  Check link attributes"""
        # the list of Pin objects linked to this signal
        assert_equal(len(self.s1.pins()), 1)

        # the name of modifying pins linked to this signal
        assert_equal(self.s1.writername, self.fqpname)
        assert_is_none(self.s1.bidirname)

        # verify the pin reflects the signal it's linked to:
        assert_true(self.p.linked)
        assert_equal(self.p.signame, self.s1.name)
        sw = self.p.signal # access through Signal() wrapper
        assert_is_not_none(sw)
        assert_is_instance(sw,hal.Signal)
        assert_equal(self.p.signal.writers, 1)

        # initial value inheritage
        assert_equal(self.s1.get(), self.p.get())

    @test(depends_on=[link])
    def link_set_fail(self):
        """Epsindex:  Setting linked signal raises exception"""
        # since now linked, self.s1.set() must fail:
        assert_raises(RuntimeError,self.s1.set,271828)

    @test(depends_on=[link])
    def signals_dictionary(self):
        """Epsindex:  Check signal dictionary"""
        print "signals: %s" % hal.signals()
        assert_equal(len(hal.signals), 1)
        assert_in(self.signame, hal.signals)
        assert_equal(hal.signals[self.signame].name, self.signame)

    @after_class
    def teardown_epsindex(self):
        """Epsindex:  Stop component"""
        hal.Signal.delete(self.signame)
        assert_equal(len(hal.signals()),0)
        self.epsindexcomp.exit()

        check_hal_clean()


@test(groups=["hal_base"])
class TestEpstest(RTAPITestCase):

    @before_class
    def init_component(self):
        """Epstest:   Initialize component"""
        # custom deltas (leave epsilon[0] - the default - untouched)
        hal.epsilon[1] = 100.0
        hal.epsilon[2] = 1000.0

        self.epstestcomp = hal.Component('epstest')

    @test
    def ccomp_and_epsilon(self):
        """Epstest:   Test pin change detection"""
        # select epsilon[1] for change detection on 'out1'
        # means: out1 must change by more than 100.0 to report a changed pin
        p1 = self.epstestcomp.newpin("out1", hal.HAL_FLOAT, hal.HAL_OUT, eps=1)

        # but use epsilon[2] - 1000.0 for out2
        p2 = self.epstestcomp.newpin("out2", hal.HAL_FLOAT, hal.HAL_OUT, eps=2)
        self.epstestcomp.ready()
        print p1.eps, p1.epsilon, p2.eps, p2.epsilon

        # we havent changed pins yet from default, so
        # self.epstestcomp.changed() reports 0 (the number of pin
        # changes detected)
        assert_equal(self.epstestcomp.changed(), 0)

        pinlist = []

        # report_all=True forces a report of all pins
        # regardless of change status, so 2 pins to report:
        self.epstestcomp.changed(userdata=pinlist,report_all=True)
        assert_equal(len(pinlist), 2)

        # passing a list as 'userdata=<list>' always clears the list before
        # appending pins
        self.epstestcomp.changed(userdata=pinlist,report_all=True)
        assert_equal(len(pinlist), 2)

        self.epstestcomp["out1"] += 101  # larger than out1's epsilon value
        self.epstestcomp["out2"] += 101  # smaller than out2's epsilon value

        # this must result in out1 reported changed, but not out2
        self.epstestcomp.changed(userdata=pinlist)
        assert_equal(len(pinlist), 1)

        self.epstestcomp["out2"] += 900  # exceed out2's epsilon value
                                         # in second update:
        self.epstestcomp.changed(userdata=pinlist)
        assert_equal(len(pinlist), 1)

        # since no changes since last report, must be 0:
        assert_equal(self.epstestcomp.changed(), 0)

    @after_class
    def teardown_class(self):
        """Epstest:   Stop component"""
        self.epstestcomp.exit()

        check_hal_clean()
