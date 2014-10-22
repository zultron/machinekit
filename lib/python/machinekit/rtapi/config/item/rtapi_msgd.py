from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool

class rtapi_app_foreground(ConfigBool):
    name = 'foreground'
    section = 'rtapi_app'
    default = False
    longopt = 'foreground'
    shortopt = 'F'
    help="Run rtapi_msgd in foreground"

class rtapi_app_instance(ConfigInt):
    name = 'instance'
    section = 'rtapi_app'
    default = 0
    longopt = 'instance'
    shortopt = 'I'
    help="RTAPI instance number"

class rtapi_app_instance_name(ConfigString):
    name = 'instance_name'
    section = 'rtapi_app'
    longopt = 'instance_name'
    shortopt = 'i'
    help="RTAPI instance name"

class rtapi_app_flavor(ConfigString):
    name = 'flavor'
    section = 'rtapi_app'
    longopt = 'flavor'
    shortopt = 'f'
    env_ok = True
    help="RTAPI flavor, e.g. posix, xenomai, rt-preempt"

class rtapi_app_log_stderr(ConfigBool):
    name = 'log_stderr'
    section = 'rtapi_app'
    default = False
    longopt = 'log_stderr'
    shortopt = 's'
    help="Log to stderr in addition to syslog"

class rtapi_app_logpub_uri(ConfigString):
    name = 'logpub_uri'
    section = 'rtapi_app'
    longopt = 'logpub_uri'
    shortopt = 'U'
    help="Logpub URI"

