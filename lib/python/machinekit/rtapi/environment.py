import os, logging

class Environment(object):
    """
    Tools for the RTAPI pre-run environment
    """
    def __init__(self, config=None, compat=None, util=None):
        self.log = logging.getLogger(self.__module__)
        self.config = config
        self.compat = compat
        self.util = util

    def assert_not_superuser(self):
        """
        Sanity check:  Refuse to execute as superuser
        """
        if os.getuid() == self.config.environment_root_gid:
            raise RTAPIEnvironmentInitError("Refusing to run as root")
        if os.geteuid() == self.config.environment_root_gid:
            raise RTAPIEnvironmentInitError("Refusing to run as setuid root")

    def assert_reasonable_rlimit_memlock(self):
        """
        Sanity check:  Assert minimal memlock limit
        """
        memlock = self.config.system_rlimit_memlock_soft/1024
        if memlock < self.config.environment_rlimit_memlock_min and \
                memlock != -1:
            raise RTAPIEnvironmentRLimitError(
                "Refusing to start:  memlock rlimit = %s, want >= %s" %
                (memlock,self.config.environment_rlimit_memlock_min))

    def assert_reasonable_rlimit_cpu(self):
        """
        Sanity check:  Assert inifinite CPU limit
        """
        cpu = self.config.system_rlimit_cpu_soft
        if cpu != -1:
            raise RTAPIEnvironmentRLimitError(
                "Refusing to start without unlimited rlimit:  cpu = %s" % cpu)

    def assert_shmdrv_writable(self):
        """
        Sanity check:  Assert /dev/shmdrv is writable
        """
        if self.config.use_shmdrv and \
                not os.access(self.config.environment_shmdrv_dev, os.W_OK):
            raise RTAPIEnvironmentPrivilegeError(
                "Refusing to start:  /dev/shmdrv required but unwritable")

    def assert_no_conflicting_kernel_instance(self):
        """
        Sanity check: Check no kthreads instance is running.  If one
        is, do some diagnosis and raise an exception.
        """
        if not self.config.use_shmdrv:
            return  # n/a

        if self.config.instance != self.compat.kernel_instance_id:
            return  # ok
        
        self.log.error(
            "Found existing kernel thread instance with same id (%d)" %
            self.config.instance)

        self.log.error("    kernel modules loaded:  %s" %
                       ', '.join([m for m in ('shmdrv', 'rtapi', 'hal_lib')
                                  if self.compat.is_module_loaded(m) ]))

        msgd_proc = self.util.proc_by_cmd("msgd:%d" % self.config.instance)
        if msgd_proc.pid > 0:
            self.log.error("    running msgd:%d process %d" %
                           (self.config.instance, msgd_proc.pid))
        else:
            self.log.error("    no running msgd:%d process found")

        raise RTAPIEnvironmentInitError(
            "Existing kernel thread instance with same ID found; see logs")

    def assert_no_conflicting_daemons(self):
        errs = []

        msgd_proc = self.util.proc_by_cmd("msgd:%d" % self.config.instance)
        if msgd_proc is not None:
            errs.append("msgd:%d (pid %d)" %
                        (self.config.instance, msgd_proc.pid))
        app_proc = self.util.proc_by_name("rtapi:%d" % self.config.instance)
        if app_proc is not None:
            errs.append("rtapi:%d (pid %d)" %
                        (self.config.instance, app_proc.pid))

        if errs:
            raise RTAPIEnvironmentInitError(
                "Found running %s with same instance as ours" %
                " and ".join(errs))

    def assert_no_conflicting_instances(self):
        # If another RTAPI instance with the same id is *running*,
        # raise an exception.
        self.assert_no_conflicting_kernel_instance()
        self.assert_no_conflicting_daemons()

    def assert_sanity(self):
        """
        Run all possible environment sanity checks before forking off
        into a daemon and doing real stuff that might fail.
        """
        self.assert_not_superuser()
        self.assert_reasonable_rlimit_memlock()
        self.assert_reasonable_rlimit_cpu()
        # FIXME:  can't read these with python-2's 'resource' module
        #   Cython bindings?
        #self.assert_reasonable_rlimit_rtprio()  min. 99
        #self.assert_reasonable_rlimit_nice()    max. -20
        self.assert_no_conflicting_instances()
        self.assert_shmdrv_writable()

class RTAPIEnvironmentInitError(RuntimeError):
    """
    This exception class is thrown for fatal errors initializing the
    real-time environment.
    """
    pass

class RTAPIEnvironmentRLimitError(RuntimeError):
    """
    Raised by flavor sanity checks when system resource limits are
    insufficient
    """
    pass

class RTAPIEnvironmentPrivilegeError(RuntimeError):
    """
    Raised by sanity checks when user has insufficient privileges to
    access real-time environment
    """
    pass


