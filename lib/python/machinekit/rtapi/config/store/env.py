from machinekit.rtapi.config.store import ConfigStore
from machinekit.rtapi.exceptions import RTAPIConfigNotFoundException
import os

class EnvStore(ConfigStore):
    """
    Environment variable configuration storage class: a read-only
    source of configuration variables
    """

    name = "env"
    priority = 15                       # comes after command line
                                        # args but before .ini files
    read_only = True                    # normally not changed

    def handles(self, obj):
        """Env handles items with attribute env_ok==True"""
        return obj.env_ok

    def env_name(self, item):
        """Translate item.name to an uppercase environment variable name"""
        return item.name.upper()

    def get(self, item):
        value = os.getenv(self.env_name(item))
        if value is None:
            raise RTAPIConfigNotFoundException(
                "item %s not found in environment" % self.env_name(item))
        return item.valtype(value)
