from machinekit.rtapi.config.item import \
    ConfigString, ConfigInt, ConfigBool, IniFileConfig

class rtapi_use_portable_parport_io(ConfigBool,IniFileConfig):
    name = 'use_portable_parport_io'
    section = 'rtapi'

class rtapi_architecture(ConfigString,IniFileConfig):
    name = 'architecture'
    section = 'rtapi'

class rtapi_git_version(ConfigString,IniFileConfig):
    name = 'git_version'
    section = 'rtapi'

class rtapi_run_in_place(ConfigBool,IniFileConfig):
    name = 'run_in_place'
    section = 'rtapi'

class rtapi_pidof(ConfigString,IniFileConfig):
    name = 'pidof'
    section = 'rtapi'

