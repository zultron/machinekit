from machinekit.rtapi.plugin import PluginLoader
from machinekit.rtapi.exceptions import RTAPIConfigException
import logging

class ConfigStore(PluginLoader):
    """
    Abstract Machinekit configuration item storage backend class

    A ConfigStore object loads all ConfigItem plugin classes from
    module files in this directory.

    Storage backend plugins arrange storage/retrieval of these
    configuration items, such as from command-line args or .ini files.
    """
    name = None                 # name of configuration store
    priority = None             # uint, 0 is highest priority

    def __init__(self):
        self.log = logging.getLogger(self.__module__)
        # Set to True when store is ready for get/set operation
        self.initialized = False

    def can_get(self, cls):
        """Return True if this config store is suitable for getting
        the section/key value"""
        return self.item_class_get_filter(self, cls)

    def get(self, section, key):
        """Get the value of section/key from this config store"""
        raise RTAPIConfigException(
            "Config store backends must implement the get() method")

    def can_set(self, cls):
        """Return True if this config store is suitable for setting
        the section/key value"""
        return self.item_class_set_filter(self, cls)

    def set(self, section, key, value):
        """Get the value of section/key in this config store"""
        raise RTAPIConfigException(
            "Config store backends must implement the set() method")

    def item_class_get_filter(self, cls):
        """can_get all config items"""
        return True

    def item_class_set_filter(self, cls):
        """can_set all config items"""
        return True

    def plugin_class_translator(self, cls):
        """Apply transformation to item class"""
        return cls

    def finalize_init(self):
        """Finalize initialization of store and mark as ready for operation"""
        self.initialized = True

    def str(self):
        return "<%s config store backend>" % (self.__class__.__name__)

    def __repr__(self):
        return self.str()

