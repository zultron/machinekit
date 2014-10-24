from machinekit.rtapi.config.item import \
    FlavorConfig, ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.item.flavor import \
    FLAVOR_DOES_IO

class flavor_var_rt_preempt(FlavorConfig,ConfigString):
    name = 'flavor_var_rt-preempt'
    value = 'rt_preempt'

class flavor_rt_preempt_name(FlavorConfig,ConfigString):
    name = 'flavor_rt_preempt_name'
    value = 'rt-preempt'

class flavor_rt_preempt_mod_ext(FlavorConfig,ConfigString):
    name = 'flavor_rt_preempt_mod_ext'
    value = '.so'

class flavor_rt_preempt_so_ext(FlavorConfig,ConfigString):
    name = 'flavor_rt_preempt_so_ext'
    value = '.so'

class flavor_rt_preempt_build_sys(FlavorConfig,ConfigString):
    name = 'flavor_rt_preempt_build_sys'
    value = 'user-dso'

class flavor_rt_preempt_id(FlavorConfig,ConfigInt):
    name = 'flavor_rt_preempt_id'
    value = 1

class flavor_rt_preempt_flags(FlavorConfig,ConfigInt):
    name = 'flavor_rt_preempt_flags'
    value = FLAVOR_DOES_IO

