from machinekit.rtapi.plugin import PluginLoader
from machinekit.rtapi.config.exceptions import RTAPIConfigException
import logging

class ConfigStore(object):
    """
    Abstract Machinekit configuration item storage backend class

    A ConfigStore object loads all ConfigItem plugin classes from
    module files in this directory.

    Storage backend plugins arrange storage/retrieval of these
    configuration items, such as from command-line args or .ini files.
    """
    name = None                 # name of configuration store
    priority = None             # uint, 0 is highest priority

    def __init__(self, config):
        self.log = logging.getLogger(self.__module__)

        # keep reference to top-level config
        self.config = config

        # add any supplied plugin configuration
        self.plugin_config = self.config.store_config.get(self.name,{})

        # register this store with each item that this store can handle
        for item in self.config.items:
            if self.handles(item):
                item.register_store(self)

    def handles(self, item):
        """True if this store can handle this ConfigItem"""
        return True  # default

    def get(self, item):
        """Get the value of section/key from this config store"""
        raise RTAPIConfigException(
            "Config store backends must implement the get() method")

    def set(self, item, value):
        """Get the value of section/key in this config store"""
        if self.read_only:
            raise RTAPIConfigException(
                "Attempt to set %s in read-only config backend %s" %
                (item.name, self.name))
        else:
            raise RTAPIConfigException(
                "Read/write config store backends must implement the "
                "set() method")

    def __str__(self):
        return self.name

    def __repr__(self):
        return "<%s config store backend>" % (self.__class__.__name__)

class ConfigStoreLoader(PluginLoader):
    """
    Machinekit configuration storage stack class

    A Config object is a stack of configuration storage backend
    plugins sorted by priority.  A storage backend knows how to filter
    config items for which its get/set operations are applicable.
    """

    pluginclass = ConfigStore

