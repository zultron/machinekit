#!/usr/bin/env python

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_equal, assert_in, assert_not_in, \
    assert_raises

from machinekit import hal

@test(groups=["hal"],
      depends_on_groups=["hal_base"])
class TestGroup(RTAPITestCase):

    @before_class
    def init_group(self):
        """SignalGroups:  Initialize signal groups"""
        self.g1 = hal.Group("group1",arg1=123,arg2=4711)
        print "new; g1.refcount: %d" % self.g1.refcount

        hal.epsilon[2] = 0.1

        assert_equal(len(hal.groups()), 1)
        assert_in("group1", hal.groups())

    @test
    def group_attributes(self):
        """SignalGroups:  Signal group attributes"""
        assert_equal(self.g1.userarg1, 123)
        assert_equal(self.g1.userarg2, 4711)

    @test
    def group_attributes_by_name(self):
        """SignalGroups:  Signal group attributes by name"""
        # access via name - second wrapper
        self.g2 = hal.Group("group1")
        assert_equal(self.g2.userarg1, 123)
        assert_equal(self.g2.userarg2, 4711)

    @test(depends_on=[group_attributes_by_name])
    def add_signals(self):
        """SignalGroups:  Add signals"""
        # add signals to group
        self.s1 = hal.Signal("sigs32",   hal.HAL_S32)
        self.s2 = hal.Signal("sigfloat", hal.HAL_FLOAT)
        self.s3 = hal.Signal("sigbit",   hal.HAL_BIT)
        self.s4 = hal.Signal("sigu32",   hal.HAL_U32)

        # by object
        self.g2.member_add(self.s1, eps_index=2)
        self.g2.member_add(self.s2)
        # by name
        self.g2.member_add("sigbit")
        self.g2.member_add("sigu32")
        print "4 sigs; g1.refcount: %d" % self.g1.refcount

        # four signals created at this point
        assert_equal(len(self.g2.signal_members()), 4)

        # adding a second signal with same name raises exception
        assert_raises(RuntimeError, self.g2.member_add, "sigu32")
        print "tried to add dup memer; g1.refcount: %d" % self.g1.refcount

        # change detection - all defaults, so no change yet
        assert_equal(len(self.g2.changed()), 0)
        print "check changed; g1.refcount: %d" % self.g1.refcount

    @test(depends_on=[add_signals])
    def change_signals(self):
        """SignalGroups:  Change member signals"""

        # change a member signal
        self.s2.set(3.14)
        print "change member sig; g1.refcount: %d" % self.g1.refcount

        # see this is reflected once
        assert_equal(len(self.g2.changed()), 1)

        # but only once
        assert_equal(len(self.g2.changed()), 0)

        self.s1.set(-112345)
        self.s2.set(2.71828)
        self.s3.set(True)
        self.s4.set(815)

        # retrieve changed values
        for s in self.g2.changed():
            print "\t",s.name,s.type,s.get(),s.writers, s.readers

        # one more group
        self.g3 = hal.Group("group3")
        self.g3.member_add(hal.Signal("someu32",   hal.HAL_U32))
        print "add new group; g1.refcount: %d" % self.g1.refcount

    @test(depends_on=[change_signals])
    def nested_signals(self):
        """SignalGroups:  Nested signal groups"""

        # add as nested group
        self.g2.member_add(self.g3)
        print "add new group to this group; g1.refcount: %d" % self.g1.refcount

        # iterate members
        for m in self.g2.members():
            # m is a Member() object
            # m.item is the object the member is referring to -
            # Signal or Group instance
            print m,m.item,m.epsilon,m.handle,m.userarg1,m.type

        # exception:  delete a signal still in a group
        assert_raises(RuntimeError,hal.Signal.delete,"sigs32")
        print "delete memer signal; g1.refcount: %d" % self.g1.refcount

    @test(depends_on=[nested_signals])
    def remove_member_signals(self):
        """SignalGroups:  Remove member signals"""

        # remove a signal from group
        self.g2.member_delete("sigs32")
        # should be four members in group now
        assert_not_in("sigs32",[s.name for s in self.g2.signal_members()])
        assert_equal(len(self.g2.signal_members()), 4)

        # delete rest of signals from group1
        self.g2.member_delete("sigfloat")
        self.g2.member_delete("sigbit")
        self.g2.member_delete("sigu32")
        self.g2.member_delete("someu32")
        assert_equal(len(self.g2.signal_members()),0)
        print "delete all signals; g1.refcount: %d" % self.g1.refcount

    @after_class
    def remove_signals_and_groups(self):
        """SignalGroups:  Remove signals"""

        # exception:  someu32 is still in group3
        assert_raises(RuntimeError,hal.Signal.delete,"someu32")

        # delete signals from group3
        self.g3.member_delete("someu32")
        print "delete other group signals; g1.refcount: %d" % self.g1.refcount

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
        print "all signals deleted; g1.refcount: %d" % self.g1.refcount

        # delete signal groups
        assert_equal(len(hal.groups()),2)
        self.g2.refcount -= 1  # incremented with g2.changed()
        self.g2.delete()
        self.g3.delete()
        print "other group deleted; g1.refcount: %d" % self.g1.refcount
        assert_equal(len(hal.groups()),0)
        
        check_hal_clean()
