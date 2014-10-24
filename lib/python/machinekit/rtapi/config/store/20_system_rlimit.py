from machinekit.rtapi.config.store import ConfigStore, \
    RTAPIConfigException, RTAPIConfigNotFoundException
from machinekit.rtapi.config.item import SystemRlimitConfig
import resource

class SystemRlimitStore(ConfigStore):
    """
    System rlimits
    """

    name = "system_rlimit"
    priority = 20                       # anything; should be no conflict
    item_class_filter = SystemRlimitConfig
                                        # handle this class of config item

    def get(self, item):
        return resource.getrlimit(
            getattr(resource, item.resource_attribute))[item.tuple_index]

    def set(self, item, value):
        # get current (soft,hard) limits
        rlimit = list(resource.getrlimit(
                getattr(resource, item.resource_attribute)))
        # update requested soft or hard limit
        rlimit[item.tuple_index] = value
        # set new (soft,hard) limits
        try:
            resource.setrlimit(
                getattr(resource, item.resource_attribute),tuple(rlimit))
        except ValueError as e:
            raise RTAPIConfigException(
                "Unable to set %s rlimit '%s' to %d: %s" %
                (item.limit_type, item.resource_attribute, value, e))
