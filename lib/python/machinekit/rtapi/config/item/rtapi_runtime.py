from machinekit.rtapi.config.item import \
    ConfigString, ConfigInt, ConfigBool, IniFileConfig, MiscConfig

class rtapi_debug(ConfigInt,IniFileConfig):
    name = 'debug'
    section = 'rtapi'
    longopt = 'debug'
    shortopt = 'd'
    default = 1
    env_ok = True
    help = "Debug level (1-5), default 1"

class rtapi_ulapi_msglevel(ConfigInt,IniFileConfig):
    name = 'ulapi_msglevel'
    section = 'rtapi'
    longopt = 'ulapi_msglevel'
    shortopt = 'u'
    default = 1
    help = "ULAPI debug message level"

class rtapi_rtapi_msglevel(ConfigInt,IniFileConfig):
    name = 'rtapi_msglevel'
    section = 'rtapi'
    longopt = 'rtapi_msglevel'
    shortopt = 'r'
    default = 1
    help = "RTAPI debug message level"

class rtapi_use_shmdrv(ConfigBool,IniFileConfig,MiscConfig):
    name = 'use_shmdrv'
    section = 'rtapi'
    longopt = 'use_shmdrv'
    filt_use_shmdrv = True      # for the 'use_shmdrv' store
    shortopt = 'S'
    help = "Use shmdrv (default for kernel thread flavors)"

class rtapi_shmdrv_opts(ConfigString,IniFileConfig):
    name = 'shmdrv_opts'
    section = 'rtapi'
    longopt = 'shmdrv_opts'
    shortopt = 'o'
    default = ''
    help = "Options to pass to shmdrv module"

class rtapi_hal_size(ConfigInt,IniFileConfig):
    name = 'hal_size'
    section = 'rtapi'
    longopt = 'hal_size'
    shortopt = 'H'
    default = 512000
    help = "HAL data segment size"

class rtapi_hal_stack_size(ConfigInt,IniFileConfig):
    name = 'hal_stack_size'
    section = 'rtapi'
    longopt = 'halstacksize'
    shortopt = 'T'
    default = 32768
    help = "HAL thread stack size"
    description = """
        default size of the thread stack size passed to rtapi_task_new() in
        hal_create_thread()
        """

class rtapi_rtlib_dir(ConfigString,IniFileConfig):
    name = 'rtlib_dir'
    section = 'rtapi'

class rtapi_libexec_dir(ConfigString,IniFileConfig):
    name = 'libexec_dir'
    section = 'rtapi'

class rtapi_bin_dir(ConfigString,IniFileConfig):
    name = 'bin_dir'
    section = 'rtapi'

