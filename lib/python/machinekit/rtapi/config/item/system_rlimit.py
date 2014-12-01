from machinekit.rtapi.config.item import \
    ConfigInt, SystemRlimitConfig, ConfigItemFactory

resources = [
    "core",
    "cpu",
    "fsize",
    "data",
    "stack",
    "rss",
    "nproc",
    "nofile",
    "ofile",
    "memlock",
    "as",
    # FIXME
    # need RTLIMIT and RTPRIO
    ]

for r in resources:
    # soft limits
    f = ConfigItemFactory(
        name = 'system_rlimit_%s_soft' % r,
        section = 'system_rlimit_soft',
        resource_attribute = 'RLIMIT_%s' % r.upper(),
        tuple_index = 0,
        limit_type = 'soft',
        bases = (SystemRlimitConfig, ConfigInt),
        )
    f.gen_config_item(globals())
    # hard limits
    f = ConfigItemFactory(
        name = 'system_rlimit_%s_hard' % r,
        section = 'system_rlimit_hard',
        resource_attribute = 'RLIMIT_%s' % r.upper(),
        tuple_index = 1,
        limit_type = 'hard',
        bases = (SystemRlimitConfig, ConfigInt),
        )
    f.gen_config_item(globals())
