from machinekit.compat import \
    RTAPIFlavorUserland, \
    RTAPIFlavorKernelException

import os

class RTAPIFlavorRTPreempt(RTAPIFlavorUserland):
    """
    RT_PREEMPT userland RTAPI threads flavor
    """
    name = "rt-preempt"
    id = 1
    prio=20                     # high prio

    # This file contains '1' if RT_PREEMPT
    preempt_rt_sysfs = "/sys/kernel/realtime"

    @property
    def assert_running_kernel_is_rt_preempt(self):
        if not os.path.exists(self.preempt_rt_sysfs):
            raise RTAPIFlavorKernelException(
                "Running kernel is not an RT_PREEMPT kernel")
        try:
            with open(self.preempt_rt_sysfs,'r') as f:
                if (int(f.readline().rstrip()) != 1):
                    raise Exception
        except:
            raise RTAPIFlavorKernelException(
                "Running kernel is not an RT_PREEMPT kernel")

    @property
    def assert_pre_runtime_environment_sanity(self):
        """
        Assertions:
        - Running RT_PREEMPT kernel
        """
        self.assert_running_kernel_is_rt_preempt
