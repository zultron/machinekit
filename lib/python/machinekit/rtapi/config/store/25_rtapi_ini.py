from machinekit.rtapi.config.store.ini import IniStore

class RTAPIIniStore(IniStore):
    """
    rtapi.ini configuration storage class (read-only)
    """
    name = 'rtapi.ini'
    priority = 25
    inifile_config = 'rtapi_ini'

    # Migrating rtapi.ini 'global' section to 'rtapi'
    section_map = {
        'rtapi' : 'global',
        }
