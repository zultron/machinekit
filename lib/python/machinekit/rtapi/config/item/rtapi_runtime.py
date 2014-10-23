from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool

class rtapi_debug(ConfigInt):
    name = 'debug'
    section = 'rtapi'
    longopt = 'debug'
    shortopt = 'd'
    default = 1
    env_ok = True
    help = "Debug level (1-5), default 1"

class rtapi_ulapi_msglevel(ConfigInt):
    name = 'ulapi_msglevel'
    section = 'rtapi'
    longopt = 'ulapi_msglevel'
    shortopt = 'u'
    help = "ULAPI debug message level"

class rtapi_rtapi_msglevel(ConfigInt):
    name = 'rtapi_msglevel'
    section = 'rtapi'
    longopt = 'rtapi_msglevel'
    shortopt = 'r'
    help = "RTAPI debug message level"

class rtapi_use_shmdrv(ConfigBool):
    name = 'use_shmdrv'
    section = 'rtapi'
    longopt = 'use_shmdrv'
    shortopt = 'S'
    help = "Use shmdrv (default for kernel thread flavors)"

class rtapi_shmdrv_opts(ConfigString):
    name = 'shmdrv_opts'
    section = 'rtapi'
    longopt = 'shmdrv_opts'
    shortopt = 'o'
    default = ''
    help = "Options to pass to shmdrv module"

class rtapi_hal_size(ConfigInt):
    name = 'hal_size'
    section = 'rtapi'
    longopt = 'hal_size'
    shortopt = 'H'
    default = 512000
    help = "HAL thread stack size"

class rtapi_rtlib_dir(ConfigString):
    name = 'rtlib_dir'
    section = 'rtapi'

class rtapi_libexec_dir(ConfigString):
    name = 'libexec_dir'
    section = 'rtapi'

class rtapi_bin_dir(ConfigString):
    name = 'bin_dir'
    section = 'rtapi'

