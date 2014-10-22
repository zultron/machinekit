from machinekit.rtapi.plugin import PluginLoader
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
    help = None
    description = None
    valtype = None
    env_ok = False   # ok to get this from environment

    def __init__(self):
        self.log = logging.getLogger(self.__module__)
        # config store backends for which this object is getter or setter
        self.getter_store = None
        self.setter_store = None

    def set_getter_store(self, store):
        self.getter_store = store

    def set_setter_store(self, store):
        self.setter_store = store

    def str(self):
        return "<ConfigItem %s/%s>" % (self.section, self.name)

    def __repr__(self):
        return self.str()

class ConfigString(ConfigItem):
    """String configuration item"""
    valtype = str
        
class ConfigInt(ConfigItem):
    """Integer configuration item"""
    valtype = int
        
class ConfigBool(ConfigItem):
    """Boolean configuration item"""
    valtype = bool

