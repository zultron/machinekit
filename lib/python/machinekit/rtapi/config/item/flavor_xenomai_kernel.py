from machinekit.rtapi.config.item import \
    FlavorConfig, ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.item.flavor import \
    FLAVOR_DOES_IO, FLAVOR_KERNEL_BUILD, FLAVOR_RTAPI_DATA_IN_SHM

class flavor_var_xenomai_kernel(FlavorConfig,ConfigString):
    name = 'flavor_var_xenomai-kernel'
    value = 'xenomai_kernel'

class flavor_xenomai_kernel_name(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_kernel_name'
    value = 'xenomai-kernel'

class flavor_xenomai_kernel_mod_ext(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_kernel_mod_ext'
    value = '.ko'

class flavor_xenomai_kernel_so_ext(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_kernel_so_ext'
    value = '.so'

class flavor_xenomai_kernel_build_sys(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_kernel_build_sys'
    value = 'kbuild'

class flavor_xenomai_kernel_id(FlavorConfig,ConfigInt):
    name = 'flavor_xenomai_kernel_id'
    value = 3

class flavor_xenomai_kernel_flags(FlavorConfig,ConfigInt):
    name = 'flavor_xenomai_kernel_flags'
    value = FLAVOR_DOES_IO + FLAVOR_KERNEL_BUILD + FLAVOR_RTAPI_DATA_IN_SHM

