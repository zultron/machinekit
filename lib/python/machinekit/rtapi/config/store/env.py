from machinekit.rtapi.config.item import ConfigString, ConfigInt, ConfigBool
from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.exceptions import RTAPIConfigNotFoundException
import os

class EnvConfigItem(object):
    """
    Env config item mixin class

    Connects config item to environment variables
    """
    def get(self):
        value = os.getenv(self.env_name)
        if value is None:
            raise RTAPIConfigNotFoundException(
                "item %s not found in environment" % self.env_name)
        return self.valtype(value)

    @property
    def env_name(self):
        """Translate self.name to an uppercase environment variable name"""
        return self.name.upper()


class EnvStore(ConfigStore):
    """
    Environment variable configuration storage class: a read-only
    source of configuration variables
    """

    name = "env"
    priority = 15                       # comes after command line
                                        # args but before .ini files
    read_only = True                    # normally not changed

    def item_class_get_filter(self, cls):
        """Env can_get items with attribute env_ok==True"""
        return cls.env_ok

    def item_class_set_filter(self, cls):
        """Env can_set no items"""
        return False

    def plugin_class_translator(self, cls):
        """Subclass config item class, adding EnvConfigItem mixin class"""
        if issubclass(cls, ConfigString):
            return type('EnvConfigString',
                        (EnvConfigItem, cls),
                        {})
        if issubclass(cls, ConfigInt):
            return type('EnvConfigInt',
                        (EnvConfigItem, cls),
                        {})
        if issubclass(cls, ConfigBool):
            return type('EnvConfigBool',
                        (EnvConfigItem, cls),
                        {})
        raise Exception ("shouldn't be here")

