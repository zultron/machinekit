from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.config.item import \
    FlavorConfig, CurrentFlavorConfig
from machinekit.rtapi.exceptions import \
    RTAPIConfigNotFoundException, RTAPIConfigException
import string

class FlavorStore(ConfigStore):
    """
    RTAPI thread flavor configuration storage class (read-only)
    """

    name = 'flavor'                     # name for storage backend
    priority = 40                       # anything above flavor_current
    read_only = True                    # flavor attributes are static
    item_class_filter = FlavorConfig    # only handle this item class

    def get(self, item):
        return item.value


class CurrentFlavorStore(ConfigStore):
    """
    This class returns data about the currently configured flavor.

    It retrieves data entirely from upper-level config layers, so the
    priority is critical.
    """

    name = 'current_flavor'
    priority = 41                       # anything below flavor_* classes
    read_only = True
    item_class_filter = \
        CurrentFlavorConfig             # only handle this item class
    
    def __init__(self, config):
        super(CurrentFlavorStore, self).__init__(config)

        # store the flavor, which may be expensive to calculate;
        # allow plugin configuration override
        self.flavor = self.plugin_config.get('flavor',config.flavor)
        self.flavor_var = config.get('flavor_var_%s' % self.flavor)

        self.log.debug("current_flavor store set current flavor to %s" %
                       self.flavor)

    def get(self, item):
        name = 'flavor_%s_%s' % (self.flavor_var, item.name_suffix)
        return self.config.get(name)
