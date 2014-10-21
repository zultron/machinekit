from machinekit.compat import \
    RTAPIFlavor, RTAPIFlavorUserland, RTAPIFlavorKernel, \
    RTAPIFlavorKernelException, RTAPIFlavorPrivilegeException, \
    RTAPIFlavorULimitException
import os, resource

class RTAPIFlavorXenomaiCommon(RTAPIFlavor):
    """
    Common Xenomai attributes and methods
    """

    # Xenomai privileged group
    xenomai_gid_sysfs = "/sys/module/xeno_nucleus/parameters/xenomai_gid"
    # XNHEAP_DEV_NAME
    xenomai_rtheap = "/dev/rtheap"
    # PROC_IPIPE_XENOMAI
    xenomai_proc_ipipe = "/proc/ipipe/Xenomai"
    # Xenomai needs high memlock ulimit than default 64k
    xenomai_min_memlock = 32767
    
    @property
    def assert_running_kernel_is_xenomai(self):
        if not (
            os.path.exists(self.xenomai_rtheap) and \
            os.path.exists(self.xenomai_proc_ipipe) and \
            os.path.exists(self.xenomai_gid_sysfs)
            ):
            raise RTAPIFlavorKernelException(
                "Running kernel is not a Xenomai kernel")

    @property
    def xenomai_gid(self):
        if not hasattr(self,"_xenomai_gid"):
            try:
                with open(self.xenomai_gid_sysfs,'r') as f:
                    self._xenomai_gid = int(f.readline().rstrip())
            except IOError as e:
                raise IOError("Unable to read Xenomai GID file: %s" % e)
            except ValueError as e:
                raise ValueError("Unable to parse read Xenomai GID: %s" % e)
        return self._xenomai_gid

    @property
    def assert_user_in_xenomai_group(self):
        if self.xenomai_gid not in os.getgroups():
            raise RTAPIFlavorPrivilegeException(
                "User not in privileged Xenomai group, GID=%d" %
                self.xenomai_gid)

    @property
    def assert_ulimit_memlock(self):
        memlock_kb = min(resource.getrlimit(resource.RLIMIT_MEMLOCK)) / 1024
        if (memlock_kb < self.xenomai_min_memlock):
            raise RTAPIFlavorULimitException(
                "Memlock ulimit = %d, need minimum = %d" %
                (memlock_kb, self.xenomai_min_memlock))

    @property
    def assert_pre_runtime_environment_sanity(self):
        """
        Assertions:
        - Running Xenomai kernel
        - User in privileged Xenomai group
        - Memlock ulimit exceeds a minimum
        """
        self.assert_running_kernel_is_xenomai
        self.assert_user_in_xenomai_group
        self.assert_ulimit_memlock
        raise Exception

class RTAPIFlavorXenomai(RTAPIFlavorXenomaiCommon, RTAPIFlavorUserland):
    """
    Xenomai userland RTAPI threads flavor
    """
    name = "xenomai"
    id = 2
    prio=10                     # high prio


class RTAPIFlavorXenomaiKernel(RTAPIFlavorXenomaiCommon, RTAPIFlavorKernel):
    """
    Xenomai kernel RTAPI threads flavor
    """
    name = "xenomai-kernel"
    id = 3
    prio=50                     # lowest prio except for POSIX (deprecated)
