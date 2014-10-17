import os, subprocess, ConfigParser
from proboscis import test, after_class, factory
from proboscis.decorators import DEFAULT_REGISTRY
from nose.plugins import Plugin
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit import rtapi, hal

@test(groups=["realtime_environment"])
class realtime_environment():
    """
    This class's methods start and stop the realtime environment.

    Tests that need RT should depend on the "realtime_start" group to
    ensure RT is started before they run, and should also belong to
    the "all_rt_tests" group to ensure they run before RT is stopped.

    These two dependencies should be taken care of in "run_tests.py".
    """

    DEBUG = "5"

    @test(groups=["realtime"])
    def realtime_start(self):
        """Start realtime"""
        e=os.environ.copy()
        e["DEBUG"] = self.DEBUG
        subprocess.call(["realtime","restart"],
                         stderr=subprocess.STDOUT, env=e)

    @test(groups=["realtime_stop"],
          runs_after_groups=["all_rt_tests"],
          always_run = True)
    def realtime_stop(self):
        """Stop realtime"""
        e=os.environ.copy()
        e["DEBUG"] = self.DEBUG
        subprocess.call(["realtime","stop"],
                        stderr=subprocess.STDOUT, env=e)


class RTAPITestCase(object):

    pdict = {   # dict to persist data in across instances
        'uuid'       : None,
        'configfile' : None,
        'config'     : None,
        'rtapi'      : None,
        }

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


def check_hal_clean():
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

def after_class_hal_clean(home=None, **kwargs):
    """
    Like @after_class decorator, but defaults to always_run=True, and
    checks HAL environment for leftovers that might interfere with
    later tests
    """
    kwargs.setdefault("always_run",True)
    kwargs['run_after_class'] = True

    # our wrapper runs the after_class wrapper and then checks the
    # environment
    if home:
        def fn_wrap_home(*iargs, **ikwargs):
            res = home(*iargs, **ikwargs)
            check_hal_clean()
            return res
        return DEFAULT_REGISTRY.register(fn_wrap_home, **kwargs)
    else:
        def cb_method(home_2):
            def fn_wrap_home(*iargs, **ikwargs):
                res = home_2(*iargs, **ikwargs)
                check_hal_clean()
                return res
            return DEFAULT_REGISTRY.register(fn_wrap_home, **kwargs)
        return cb_method


