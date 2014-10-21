from machinekit.compat import \
    RTAPIFlavorUserland, RTAPIFlavorKernel, \
    RTAPIFlavorKernelException, RTAPIFlavorPrivilegeException, \
    RTAPIFlavorULimitException
import os, re, resource

class RTAPIFlavorRTAIKernel(RTAPIFlavorKernel):
    """
    RTAI kernel RTAPI threads flavor
    """
    name = "rtai-kernel"
    prio = 20                     # high prio

    proc_ipipe = "/proc/ipipe"
    # RTAI kernel symbol regex to search; 'rt_daemonize' for older
    # RTAI, 'rtai_irq_handler' for newer
    rtai_ksym_re = re.compile(r'(rt_daemonize|rtai_irq_handler)')

    @property
    def assert_running_kernel_is_rtai(self):
        if not (os.path.exists(self.proc_ipipe)):
            raise RTAPIFlavorKernelException(
                "Running kernel is not an RTAI kernel")
        with open("/proc/kallsyms" % self.kernel_source_dir,'r') as f:
            for line in f:
                if r.search(line):
                    break
            else:
                raise RTAPIFlavorKernelException(
                    "Running kernel is not an RTAI kernel")

    @property
    def assert_pre_runtime_environment_sanity(self):
        """
        Assertions:
        - Running RTAI kernel
        """
        self.assert_running_kernel_is_rtai
