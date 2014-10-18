import os, subprocess, ConfigParser

from nose.tools import assert_equal

from machinekit import rtapi, hal

env = {
    #"DEBUG" : "5",
    #"MSGD_OPTS" : "-s",
}

def setup_package():
    """
    Start RT environment
    """
    e=os.environ.copy()
    e.update(env)
    subprocess.call(["realtime","restart"],
                    stderr=subprocess.STDOUT, env=e)

def teardown_package():
    """Stop realtime"""
    e=os.environ.copy()
    e.update(env)
    subprocess.call(["realtime","stop"],
                    stderr=subprocess.STDOUT, env=e)


#################
# RTAPITestCase

class Fixture(object):
    """
    Provides object persistence between tests within a test class.
    """
    pass

fixture_dict = {}

class RTAPITestCase(object):
    """
    This class should be subclassed by other tests.

    It provides three things:

    1) A 'self.rtapi' attribute that is persistent across the entire
    'nosetests' run, across classes and modules.  This enables the
    realtime environment to be initialized once per 'nosetests' run,
    and 'rtapi.RTAPIcommand()' run once (on demand) to establish a
    connection (this command fails if run multiple times).

    2) A 'test_99999_check_hal_clean' function that automatically
    checks HAL for objects leftover by a test class.  Two benefits are
    ensuring that tear down is tested for all test classes, and the
    HAL environment is clean of anything that may interfere with the
    next class's tests.

    3) A 'self.f' fixture object whose attributes remain persistent
    between tests within a class.  The 'nosetests' system
    reinitializes the test class object between each test, so setting
    a 'self.foo' object in one test doesn't persist to the next test.
    """

    pdict = {   # dict to persist data in across instances
        'uuid'       : None,
        'configfile' : None,
        'config'     : None,
        'rtapi'      : None,
        }

    def __init__(self):
        self.f = fixture_dict.setdefault(self.__class__.__name__,Fixture())
        super(RTAPITestCase,self).__init__()

    @property
    def configfile(self):
        if self.pdict['configfile'] is None:
            self.pdict['configfile'] = os.getenv("MACHINEKIT_INI")
        return self.pdict['configfile']

    @property
    def config(self):
        if self.pdict['config'] is None:
            self.pdict['config'] = ConfigParser.ConfigParser()
            self.pdict['config'].read(self.configfile)
        return self.pdict['config']

    @property
    def uuid(self):
        if self.pdict['uuid'] is None:
            self.pdict['uuid'] = \
                self.config.get("MACHINEKIT", "MKUUID")
        return self.pdict['uuid']

    @property
    def rtapi(self):
        if self.pdict['rtapi'] is None:
            self.pdict['rtapi'] = \
                rtapi.RTAPIcommand(uuid=self.uuid)
        return self.pdict['rtapi']


    def test_99999_check_hal_clean(self):
        """Teardown: Check HAL for leftover objects"""
        assert_equal(len(hal.components()),1,
                     "HAL components still exist: %s" % hal.components())
        assert_equal(len(hal.pins()),0,
                     "HAL pins still exist: %s" % hal.pins())
        assert_equal(len(hal.signals()),0,
                     "HAL signals still exist: %s" % hal.signals())
        assert_equal(len(hal.groups()),0,
                     "HAL signal groups still exist: %s" % hal.groups())
        assert_equal(len(hal.rings()),0,
                     "HAL ring buffers still exist: %s" % hal.rings())

