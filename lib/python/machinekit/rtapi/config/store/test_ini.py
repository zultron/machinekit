from machinekit.rtapi.config.store.ini import IniStore

class TestIniStore(IniStore):
    """
    'test.ini' configuration storage class (read-only) for testing
    """

    name = "test.ini"
    priority = 30                       # normally follows command
                                        # line and environment
    disabled = True                     # normally disabled
    inifile_config = 'inifile'
                                        # reuse machinekit.ini filename config

    # Test migration
    section_map = {
        'service' : 'MACHINEKIT',
        'rtapi' : 'global',
        }
