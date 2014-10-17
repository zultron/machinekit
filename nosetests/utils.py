import os, ConfigParser

from nose.tools import assert_equal

from machinekit import rtapi, hal

class Fixture(object):
    """
    Provides object persistence within a test class, between tests
    """
    pass

fixture_dict = {}

class RTAPITestCase(object):

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


    def test_99_check_hal_clean(self):
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

