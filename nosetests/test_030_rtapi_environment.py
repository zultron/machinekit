from . import FixtureTestCase, FakeHarness
from nose.plugins.attrib import attr
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit.rtapi import *
import os

# A lot of these are dopey tests, but the test harness needs a little
# work before some of these can be tested properly.

class test_030_rtapi_environment(FixtureTestCase):

    def test_03010_init_environment_object(self):
        """03010 rtapi environment:  init environment object"""
        
        # set up config fixture with 'test' store config dict for live
        # manipulation
        self.fix(
            config = Config(enabled_stores=['test'],
                            store_config = {'test' : {'instance' : 0}}))
        # add the 'test' store config dict for live manipulation
        self.fix(
            config_dict = self.config.stores.by_name('test').config_dict)

        # set up env fixture with fake compat and util harnesses
        self.fix(
            fake_compat = FakeHarness(),
            fake_util = FakeHarness(),
            # fake_util = FakeUtil()
            )
        self.fix(
            env = Environment(config=self.config,
                              compat=self.fake_compat,
                              util=self.fake_util))

        # sanity check:  instance == 0
        assert_equal(self.config.instance,0)

    def test_03020_assert_not_superuser(self):
        """03020 rtapi environment:  assert not superuser"""
        # sanity checks
        my_uid = os.getuid()
        assert_greater(my_uid,0)  # running as root will break setuid tests

        # assert not setuid
        self.env.assert_not_superuser()

        # fake being setuid and check the exception is raised
        # again, faking being setuid:  raise exception
        self.config_dict['environment_root_gid'] = my_uid
        assert_raises(RTAPIEnvironmentInitError,
                      self.env.assert_not_superuser)

        # reset config
        self.config_dict.pop('environment_root_gid')

    def test_03030_assert_reasonable_rlimit_memlock(self):
        """03030 rtapi environment:  assert reasonable rlimit memlock"""
        # these tests fake memlock ulimit scenarios

        # pass:  set > min
        self.config_dict['system_rlimit_memlock_soft'] = 20000*1024
        self.env.assert_reasonable_rlimit_memlock()

        # check exception:  set < min
        self.config_dict['system_rlimit_memlock_soft'] = 20000*1024-1
        assert_raises(RTAPIEnvironmentRLimitError,
                      self.env.assert_reasonable_rlimit_memlock)

        # check pass:  set == infinity
        self.config_dict['system_rlimit_memlock_soft'] = -1
        self.env.assert_reasonable_rlimit_memlock()
        
        # clean up
        self.config_dict.pop('system_rlimit_memlock_soft')

    def test_03031_assert_reasonable_rlimit_cpu(self):
        """03031 rtapi environment:  assert reasonable rlimit cpu"""
        # these tests fake cpu ulimit scenarios

        # check pass:  set == infinity
        self.config_dict['system_rlimit_cpu_soft'] = -1
        self.env.assert_reasonable_rlimit_cpu()

        # check exception:  set != infinity
        self.config_dict['system_rlimit_cpu_soft'] = 20000*1024
        assert_raises(RTAPIEnvironmentRLimitError,
                      self.env.assert_reasonable_rlimit_cpu)
        
        # clean up
        self.config_dict.pop('system_rlimit_cpu_soft')

    def test_03040_assert_dev_shmdrv_writable(self):
        """03040 rtapi environment:  assert /dev/shmdrv is writable"""
        # these tests stand /dev/null (writable) and /dev/rtc0 (not
        # writable) in place of /dev/shmdrv to test logic

        # check pass:  writable device and use_shmdrv is False
        self.config_dict['environment_shmdrv_dev'] = '/dev/null'
        self.config_dict['use_shmdrv'] = False
        self.env.assert_shmdrv_writable()

        # check pass:  writable device and use_shmdrv is True
        self.config_dict['environment_shmdrv_dev'] = '/dev/null'
        self.config_dict['use_shmdrv'] = True
        self.env.assert_shmdrv_writable()

        # check exception:  unwritable device and use_shmdrv is True
        self.config_dict['environment_shmdrv_dev'] = '/dev/rtc0'
        self.config_dict['use_shmdrv'] = True
        assert_raises(RTAPIEnvironmentPrivilegeError,
                      self.env.assert_shmdrv_writable)

        # reset config
        self.config_dict.pop('environment_shmdrv_dev')
        self.config_dict.pop('use_shmdrv')

    def test_03050_assert_no_conflicting_daemons(self):
        """03050 rtapi environment:  no conflicting daemons"""
        # these tests fake running msgd:0 and rtapi:0 processes

        # No ID conflict:  id = 1
        self.config_dict['instance'] = 1
        # Set up fake procs:  no msgd:1 or rtapi:1
        self.fake_util.add_simple_methods(
            proc_by_cmd = None,
            proc_by_name = None)
        assert_is_none(self.env.util.proc_by_name(
                'rtapi:%d' % self.config.instance))
        assert_is_none(self.fake_util.proc_by_cmd(
                'msgd:%d' % self.config.instance))
        self.env.assert_no_conflicting_daemons()

        # ID cnflict:  id = 0
        self.config_dict['instance'] = 0
        # Set up fake procs:  msgd:0, pid=13; rtapi:0, pid=42
        self.fake_util.add_simple_methods(
            proc_by_name = self.fake_util.fake_object(pid=42),
            proc_by_cmd = self.fake_util.fake_object(pid=13))
        assert_equal(self.fake_util.proc_by_name(
                'rtapi:%d' % self.config.instance).pid, 42)
        assert_equal(self.fake_util.proc_by_cmd(
                'msgd:%d' % self.config.instance).pid, 13)
        assert_raises(RTAPIEnvironmentInitError,
                      self.env.assert_no_conflicting_daemons)


    def test_03051_assert_no_conflicting_kernel_instance(self):
        """03051 rtapi environment:  no conflicting kernel thread instance"""
        # these tests test logic by setting up scenarios with the fake
        # compat and util objects passed to self.env

        # Init
        # Set up a fake function needed halfway through
        self.fake_compat.add_simple_methods(is_module_loaded=True)


        # Setup:  use_shmdrv = False
        self.config_dict['use_shmdrv'] = False
        assert_false(self.config.use_shmdrv)

        # Test:  no exception
        self.env.assert_no_conflicting_kernel_instance()

        # Setup:  use_shmdrv = True
        self.config_dict['use_shmdrv'] = True
        assert_true(self.config.use_shmdrv)

        # Setup:  No ID conflict
        #   fake kthread instance = 1, config instance = 0
        self.fake_compat.add_simple_properties(kernel_instance_id=1)
        assert_equal(self.env.compat.kernel_instance_id, 1)
        self.config_dict['instance'] = 0
        assert_equal(self.config.instance, 0)

        # Test:  no exception
        self.env.assert_no_conflicting_kernel_instance()

        # Setup:  ID conflict: fake kthread instance == config instance == 0
        self.fake_compat.add_simple_properties(kernel_instance_id=0)
        assert_equal(self.env.compat.kernel_instance_id, 0)
        assert_equal(self.config.instance, 0)

        # Setup:  msgd:0 pid=0
        self.fake_util.add_simple_methods(
            proc_by_cmd = self.fake_util.fake_object(pid=0))
        assert_equal(self.fake_util.proc_by_cmd(
                'msgd:%d' % self.config.instance).pid, 0)

        # Test:  exception
        assert_raises(RTAPIEnvironmentInitError,
                      self.env.assert_no_conflicting_kernel_instance)

        # Setup:  msgd:0 pid=13
        # This test only differs from the previous in the logs
        self.fake_util.add_simple_methods(
            proc_by_cmd = self.fake_util.fake_object(pid=13))
        assert_equal(self.fake_util.proc_by_cmd(
                'msgd:%d' % self.config.instance).pid, 13)

        # Test:  exception
        assert_raises(RTAPIEnvironmentInitError,
                      self.env.assert_no_conflicting_kernel_instance)
