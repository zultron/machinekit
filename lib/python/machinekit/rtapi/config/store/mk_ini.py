from machinekit.rtapi.config.store.ini import IniStore

class MKIniStore(IniStore):
    """
    'machinekit.ini' configuration storage class (read-only)
    """

    name = "machinekit.ini"
    priority = 30                       # normally follows command
                                        # line and environment
    inifile_config = ('rtapi_config','inifile')
                                        # machinekit.ini filename config item

    # Migrating machinekit.ini 'machinekit' section to 'service'
    section_map = {
        'service' : 'MACHINEKIT'
        }
