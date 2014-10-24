from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool, \
    CurrentFlavorConfig

# These items are used by the FooFlavor config stores.

# Flags used in the flavor_foo_flags classes
FLAVOR_DOES_IO = 1
FLAVOR_KERNEL_BUILD = 2
FLAVOR_RTAPI_DATA_IN_SHM = 4


# These items are used by the CurrentFlavorStore to return config data
# about the current flavor, whatever it may be

class flavor_name(CurrentFlavorConfig,ConfigString):
    name = 'flavor_name'
    name_suffix = 'name'

class flavor_mod_ext(CurrentFlavorConfig,ConfigString):
    name = 'flavor_mod_ext'
    name_suffix = 'mod_ext'

class flavor_so_ext(CurrentFlavorConfig,ConfigString):
    name = 'flavor_so_ext'
    name_suffix = 'so_ext'

class flavor_build_sys(CurrentFlavorConfig,ConfigString):
    name = 'flavor_build_sys'
    name_suffix = 'build_sys'

class flavor_id(CurrentFlavorConfig,ConfigInt):
    name = 'flavor_id'
    name_suffix = 'id'

class flavor_flags(CurrentFlavorConfig,ConfigInt):
    name = 'flavor_flags'
    name_suffix = 'flags'
