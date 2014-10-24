from machinekit.rtapi.config.item import \
    FlavorConfig, ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.item.flavor import \
    FLAVOR_DOES_IO

class flavor_var_xenomai(FlavorConfig,ConfigString):
    name = 'flavor_var_xenomai'
    value = 'xenomai'

class flavor_xenomai_name(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_name'
    value = 'xenomai'

class flavor_xenomai_mod_ext(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_mod_ext'
    value = '.so'

class flavor_xenomai_so_ext(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_so_ext'
    value = '.so'

class flavor_xenomai_build_sys(FlavorConfig,ConfigString):
    name = 'flavor_xenomai_build_sys'
    value = 'user-dso'

class flavor_xenomai_id(FlavorConfig,ConfigInt):
    name = 'flavor_xenomai_id'
    value = 2

class flavor_xenomai_flags(FlavorConfig,ConfigInt):
    name = 'flavor_xenomai_flags'
    value = FLAVOR_DOES_IO

