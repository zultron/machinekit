# These are RTAPI system environment items that would probably be
# hard-coded, but keeping them here gives the test harness a lot of
# power over simulating different conditions.
#
# They're very simple:  just a name, default value, and data type.

from machinekit.rtapi.config.item import RTAPIConfigException, \
    ConfigString, ConfigInt, ConfigBool, ConfigItemFactory

items = [
    # environment.assert_not_superuser
    ('environment_root_gid', 0),
    # environment.assert_reasonable_rlimit_memlock
    ('environment_rlimit_memlock_min', 20000),
    # environment.assert_shmdrv_writable
    ('environment_shmdrv_dev', '/dev/shmdrv'),
    ]




# Generate ConfigItems from the above list
for i in items:
    if isinstance(i[1], str):
        base = ConfigString
    elif isinstance(i[1], int):
        base = ConfigInt
    elif isinstance(i[1], bool):
        base = ConfigBool
    else:
        raise RTAPIConfigException(
            "Unhandled class in rtapi_environment config item module")

    f = ConfigItemFactory(
        name = i[0],
        section = 'rtapi_environment',
        default = i[1],
        bases = (base,),
        )
    f.gen_config_item(globals())
