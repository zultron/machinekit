import os, sys, ConfigParser, threading, Queue, subprocess

from nose.tools import assert_equal

from machinekit import rtapi, hal

import logging, logging.config, sys
logging.config.fileConfig(os.path.join(os.path.dirname(__file__),'logging.conf'))

class Fixture(object):
    """
    Provides object persistence between tests within a test class.
    """
    pass

fixture_dict = {}

class FixtureTestCase(object):
    """
    This test case class provides:

    1) A 'self.fix(attr1=val1, attr2=val2)' method whose attributes
    become class attributes, persistent between tests within a class.
    The 'nosetests' system reinitializes the test class object between
    each test, so setting a 'self.foo' object in one test doesn't
    persist to the next test; this skirts that limitation by setting
    the attributes in the class, instead..

    2) A 'self.log' logger object.

    3) A 'filesdir' path.
    """

    filesdir = os.path.realpath(os.path.join(
            os.path.dirname(__file__),'files'))
    nosetestsdir = os.path.realpath(os.path.dirname(__file__))

    def __init__(self):
        super(FixtureTestCase,self).__init__()

        # DEPRECATED
        self.f = fixture_dict.setdefault(self.__class__.__name__,Fixture())

        logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
        self.log = logging.getLogger('nosetestLogger')
        self.log.setLevel(logging.DEBUG)

        # Add nosetestsdir to python path
        if self.nosetestsdir not in sys.path:
            sys.path.append(self.nosetestsdir)

    @classmethod
    def add_fixture(cls,name,obj):
        """
        Add a new attribute in the class scope that will survive
        across class instances
        """
        # if hasattr(cls,name):
        #     print (dir(cls))
        #     raise AttributeError(
        #         "Attempt to add existing attribute '%s' to class '%s'" %
        #         (name, cls.__name__))
        setattr(cls,name,obj)

    @classmethod
    def fix(cls,**kwargs):
        """
        Add new attributes to the class scope that will survive across
        class instances
        """
        # if hasattr(cls,name):
        #     print (dir(cls))
        #     raise AttributeError(
        #         "Attempt to add existing attribute '%s' to class '%s'" %
        #         (name, cls.__name__))
        for key in kwargs:
            setattr(cls,key,kwargs[key])

class RTAPITestCase(FixtureTestCase):
    """
    This test case class provides RTAPI and HAL functions on top of
    the 'FixtureTestCase' class:

    2) A lazily-created 'self.rtapi' attribute that is persistent
    across the entire 'nosetests' run, across classes and modules.
    This enables the realtime environment to be initialized once per
    'nosetests' run, and 'rtapi.RTAPIcommand()' run once (since it
    fails if run multiple times) at first reference to establish a
    connection.  The first reference should be during the '100_rtapi'
    tests.

    3) A 'test_99999_check_hal_clean' function that automatically
    checks HAL for objects leftover by a test class.  Two benefits are
    ensuring that tear down is tested for all test classes, and the
    HAL environment is clean of anything that may interfere with the
    next class's tests.

    """

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


    def test_99010_check_hal_clean(self):
        """      Teardown: Check HAL for leftover objects"""
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

runwithlog_threads = []  # list of running RunWithLog objects that
                         # need join()ing at shutdown


class FakeHarness(object):
    """
    This class pretends to be another class for simulating conditions
    in a test.  It can fake these things:

    Simple method: an object method that takes any arguments and
    always returns the same value

    Simple property:  a property that always returns the same value

    Arg method: an object method that returns a value from a given
    set of command-line args

    Fake object: returns an object with attributes set from the
    __init__ function **kwargs.
    """

    class _simple_method_descriptor(object):
        def __init__(self, name, value):
            print "initializing method %s, value %s" % (name, value)
            self.name = name
            self.value = value
        def _closure(self, *args, **kwargs):
            if hasattr(self.value,'__dict__'):
                print "simple method:  called %s; returning obj %s" % \
                    (self.name, self.value.__dict__)
            else:
                print "simple method:  called %s; returning %s" % \
                    (self.name, self.value)
            return self.value
        def __get__(self, obj, objtype=None):
            return self._closure

    def add_simple_methods(self, **kwargs):
        for key in kwargs:
            print "setting method %s, value %s" % (key, kwargs[key])
            setattr(self.__class__,
                    key, self._simple_method_descriptor(key, kwargs[key]))

    class _simple_property_descriptor(object):
        def __init__(self,value):
            self.value = value
        def __get__(self, obj, objtype=None):
            return self.value

    def add_simple_properties(self, **kwargs):
        for key in kwargs:
            setattr(self.__class__,
                    key, self._simple_property_descriptor(kwargs[key]))

    class _arg_method_descriptor(object):
        def __init__(self, **kwargs):
            self.result_dict = {}
            for key in kwargs:
                value = kwargs[key]
                if isinstance(key, tuple):
                    self.result_dict[key] = value
                else:
                    self.result_dict[(key,)] = value
        def __get__(self, obj, objtype=None):
            return lambda *args: self.result_dict[args]

    def add_dict_method(self, name, **kwargs):
        setattr(self.__class__,
                name, self._dict_method_descriptor(**kwargs))

    class _fake_object(object):
        def __init__(self, **kwargs):
            for attr in kwargs:
                setattr(self, attr, kwargs[attr])
    @classmethod
    def fake_object(cls, **kwargs):
        return cls._fake_object(**kwargs)

class RunWithLog(threading.Thread):
    """
    Run a command in a thread, optionally setting extra environment
    variables, directing stdout to a log file.
    """
    def __init__(self, cmd, fname, env_update={}):
        super(RunWithLog, self).__init__()
        self.cmd = cmd
        self.fname = fname
        self.env = os.environ.copy()
        self.env.update(env_update)
        #self.daemon = True

    def run(self):
        runwithlog_threads.append(self)
        self.proc = subprocess.Popen(self.cmd,
                                     stderr=subprocess.PIPE,
                                     bufsize=1,
                                     close_fds=True,
                                     env=self.env)

        with open(self.fname,'w') as fd:
            while True:
                for line in iter(self.proc.stderr.readline, ''):
                    fd.write(line)
                code = self.proc.poll()
                if code is not None:
                    fd.close()
                    return code

    @classmethod
    def wait_for_finish(cls):
        for t in runwithlog_threads:
            t.join()

class Run(object):
    """
    Run a command
    """
    @classmethod
    def simple(cls, cmd, env_update={}):
        env = os.environ.copy()
        env.update(env_update)
        return subprocess.call(cmd, env=env)


def teardown_package():
    """
    Stop realtime after tests.  This duplicates test_99999_stop_rt,
    but depending on how nosetests is run, that won't always be
    called.
    """
    subprocess.call(["realtime","stop"],
                    stderr=subprocess.STDOUT)
