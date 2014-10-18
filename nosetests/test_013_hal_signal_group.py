#!/usr/bin/env python

from . import RTAPITestCase
from nose.tools import assert_equal, assert_in, assert_not_in, \
    assert_raises

from machinekit import hal

class test_013_hal_signal_group(RTAPITestCase):

    def test_01310_init_group(self):
        """01310 hal signal group:  Initialize signal groups"""
        self.f.g1 = hal.Group("group1",arg1=123,arg2=4711)
        print "new; g1.refcount: %d" % self.f.g1.refcount

        hal.epsilon[2] = 0.1

        assert_equal(len(hal.groups()), 1)
        assert_in("group1", hal.groups())

    def test_01320_group_attributes(self):
        """01320 hal signal group:  Signal group attributes"""
        assert_equal(self.f.g1.userarg1, 123)
        assert_equal(self.f.g1.userarg2, 4711)

    def test_01321_group_attributes_by_name(self):
        """01321 hal signal group:  Signal group attributes by name"""
        # access via name - second wrapper
        self.f.g2 = hal.Group("group1")
        assert_equal(self.f.g2.userarg1, 123)
        assert_equal(self.f.g2.userarg2, 4711)

    def test_01330_add_signals(self):
        """01330 hal signal group:  Add signals"""
        # add signals to group
        self.f.s1 = hal.Signal("sigs32",   hal.HAL_S32)
        self.f.s2 = hal.Signal("sigfloat", hal.HAL_FLOAT)
        self.f.s3 = hal.Signal("sigbit",   hal.HAL_BIT)
        self.f.s4 = hal.Signal("sigu32",   hal.HAL_U32)

        # by object
        self.f.g2.member_add(self.f.s1, eps_index=2)
        self.f.g2.member_add(self.f.s2)
        # by name
        self.f.g2.member_add("sigbit")
        self.f.g2.member_add("sigu32")
        print "4 sigs; g1.refcount: %d" % self.f.g1.refcount

        # four signals created at this point
        assert_equal(len(self.f.g2.signal_members()), 4)

        # adding a second signal with same name raises exception
        assert_raises(RuntimeError, self.f.g2.member_add, "sigu32")
        print "tried to add dup memer; g1.refcount: %d" % self.f.g1.refcount

        # change detection - all defaults, so no change yet
        assert_equal(len(self.f.g2.changed()), 0)
        print "check changed; g1.refcount: %d" % self.f.g1.refcount

    def test_01331_change_signals(self):
        """01331 hal signal group:  Change member signals"""

        # change a member signal
        self.f.s2.set(3.14)
        print "change member sig; g1.refcount: %d" % self.f.g1.refcount

        # see this is reflected once
        assert_equal(len(self.f.g2.changed()), 1)

        # but only once
        assert_equal(len(self.f.g2.changed()), 0)

        self.f.s1.set(-112345)
        self.f.s2.set(2.71828)
        self.f.s3.set(True)
        self.f.s4.set(815)

        # retrieve changed values
        for s in self.f.g2.changed():
            print "\t",s.name,s.type,s.get(),s.writers, s.readers

        # one more group
        self.f.g3 = hal.Group("group3")
        self.f.g3.member_add(hal.Signal("someu32",   hal.HAL_U32))
        print "add new group; g1.refcount: %d" % self.f.g1.refcount

    def test_01340_nested_signals(self):
        """01340 hal signal group:  Nested signal groups"""

        # add as nested group
        self.f.g2.member_add(self.f.g3)
        print "add new group to this group; g1.refcount: %d" % \
            self.f.g1.refcount

        # iterate members
        for m in self.f.g2.members():
            # m is a Member() object
            # m.item is the object the member is referring to -
            # Signal or Group instance
            print m,m.item,m.epsilon,m.handle,m.userarg1,m.type

        # exception:  delete a signal still in a group
        assert_raises(RuntimeError,hal.Signal.delete,"sigs32")
        print "delete memer signal; g1.refcount: %d" % self.f.g1.refcount

    def test_01350_remove_member_signals(self):
        """01350 hal signal group:  Remove member signals"""

        # remove a signal from group
        self.f.g2.member_delete("sigs32")
        # should be four members in group now
        assert_not_in("sigs32",[s.name for s in self.f.g2.signal_members()])
        assert_equal(len(self.f.g2.signal_members()), 4)

        # delete rest of signals from group1
        self.f.g2.member_delete("sigfloat")
        self.f.g2.member_delete("sigbit")
        self.f.g2.member_delete("sigu32")
        self.f.g2.member_delete("someu32")
        assert_equal(len(self.f.g2.signal_members()),0)
        print "delete all signals; g1.refcount: %d" % self.f.g1.refcount

    def test_01360_remove_signals_and_groups(self):
        """01360 hal signal group:  Remove signals"""

        # exception:  someu32 is still in group3
        assert_raises(RuntimeError,hal.Signal.delete,"someu32")

        # delete signals from group3
        self.f.g3.member_delete("someu32")
        print "delete other group signals; g1.refcount: %d" % \
            self.f.g1.refcount

        # delete signals
        print "Signal list before deleting:  %s" % hal.signals()
        assert_equal(len(hal.signals()),5)
        hal.Signal.delete("sigs32")
        hal.Signal.delete("sigfloat")
        hal.Signal.delete("sigbit")
        hal.Signal.delete("sigu32")
        hal.Signal.delete("someu32")
        print "Signal list after deleting:  %s" % hal.signals()
        assert_equal(len(hal.signals()),0)
        print "all signals deleted; g1.refcount: %d" % self.f.g1.refcount

        # delete signal groups
        assert_equal(len(hal.groups()),2)
        self.f.g2.refcount -= 1  # incremented with g2.changed()
        self.f.g2.delete()
        self.f.g3.delete()
        print "other group deleted; g1.refcount: %d" % self.f.g1.refcount
        assert_equal(len(hal.groups()),0)
