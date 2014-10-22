from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool

class rtapi_config_inifile(ConfigString):
    name = 'inifile'
    section = 'rtapi_config'
    default = '/etc/linuxcnc/machinekit.ini'
    longopt = 'inifile'
    shortopt = 'M'
    help="Path to Machinekit .ini file"

class rtapi_config_rtapi_ini(ConfigString):
    name = 'rtapi_ini'
    section = 'rtapi_config'
    default = '/etc/linuxcnc/rtapi.ini'
    longopt = 'rtapi_ini'
    shortopt = 'c'
    help="Path to rtapi.ini file"
