from machinekit.rtapi.plugin import PluginLoader
from machinekit.rtapi.config.exceptions import RTAPIConfigNotFoundException
import logging

class ConfigItem(object):
    """
    Configuration item storage object abstract class.

    Each configuration item, such as service/service_uuid or
    rtapi/rtapi_debug, has its own class, a subclass of this one.  The
    item's static properties, like section/name, are class attributes,
    and the item's value is the instantiated object's attribute.
    """
    name = None
    section = None
    longopt = None
    shortopt = None
    default = None
    help = None
    description = None
    valtype = None
    env_ok = False   # ok to get this from environment

    def __init__(self, config):
        self.log = logging.getLogger(self.__module__)
        # save reference to top-level config and add ourself to the index
        self.config = config
        self.config.index_add(self)
        # config store backend for this object
        self.store_list = []
        self._store = None

        # add any supplied plugin configuration
        self.plugin_config = self.config.item_config.get(self.name,{})

    def register_store(self, store):
        self.store_list.append(store)

    def store(self, default=None):
        # The first time, sift through stores to find our value.  If
        # a working store is found, remember it for next time.
        if self._store is None:
            for store in self.store_list:
                try:
                    store.get(self) # Raises exception if not found
                    self._store = store
                    return store
                except RTAPIConfigNotFoundException:
                    continue
            else:
                if default:
                    return default
                else:
                    raise RTAPIConfigNotFoundException(
                        "Config item %s/%s not found in any config store" %
                        (self.section, self.name))
        else:
            return self._store

    def get(self):
        try:
            return self.store().get(self)
        except RTAPIConfigNotFoundException:
            return self.default

    @property
    def initialized(self):
        return self.store().initialized

    def __str__(self):
        return "%s/%s" % (self.section, self.name)

    def __repr__(self):
        return "<ConfigItem %s/%s>" % (self.section, self.name)

class ConfigString(ConfigItem):
    """String configuration item"""
    valtype = str
        
class ConfigInt(ConfigItem):
    """Integer configuration item"""
    valtype = int
        
class ConfigBool(ConfigItem):
    """Boolean configuration item"""
    valtype = bool


class ConfigItemLoader(PluginLoader):
    """
    Machinekit configuration item pool class

    This class loads a pool of config items (e.g. service/service_uuid
    or rtapi/rtapi_debug) implemented as plugin classes.  Each config
    item is a subclass of the abstract ConfigItem class.

    These config items are matched with config storage backend stacks
    to handle get/set operations.
    """

    pluginclass = ConfigItem
    flagattr = 'name'
    no_sort = True

