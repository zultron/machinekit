from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.config.item import \
    MiscConfig
from machinekit.rtapi.exceptions import \
    RTAPIConfigNotFoundException, RTAPIConfigException

class UseShmDrvStore(ConfigStore):
    name = 'use_shmdrv'
    priority = 50       # depends on 'flavor_build_sys' and anything
                        # that can override 'use_shmdrv'
    read_only = True
    item_class_filter = MiscConfig
    item_attr_filter = 'filt_use_shmdrv'

    def get(self, item):
        return self.config.flavor_build_sys == 'kbuild'

