from machinekit.rtapi.config.item import \
    FlavorConfig, ConfigString, ConfigInt, ConfigBool

class flavor_var_posix(FlavorConfig,ConfigString):
    name = 'flavor_var_posix'
    value = 'posix'

class flavor_posix_name(FlavorConfig,ConfigString):
    name = 'flavor_posix_name'
    value = 'posix'

class flavor_posix_mod_ext(FlavorConfig,ConfigString):
    name = 'flavor_posix_mod_ext'
    value = '.so'

class flavor_posix_so_ext(FlavorConfig,ConfigString):
    name = 'flavor_posix_so_ext'
    value = '.so'

class flavor_posix_build_sys(FlavorConfig,ConfigString):
    name = 'flavor_posix_build_sys'
    value = 'user-dso'

class flavor_posix_id(FlavorConfig,ConfigInt):
    name = 'flavor_posix_id'
    value = 0

class flavor_posix_flags(FlavorConfig,ConfigInt):
    name = 'flavor_posix_flags'
    value = 0

