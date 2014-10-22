from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool

class rtapi_use_portable_parport_io(ConfigBool):
    name = 'use_portable_parport_io'
    section = 'rtapi'

class rtapi_architecture(ConfigString):
    name = 'architecture'
    section = 'rtapi'

class rtapi_git_version(ConfigString):
    name = 'git_version'
    section = 'rtapi'

class rtapi_run_in_place(ConfigBool):
    name = 'run_in_place'
    section = 'rtapi'

class rtapi_pidof(ConfigString):
    name = 'pidof'
    section = 'rtapi'

