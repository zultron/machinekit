# Config store exceptions
from machinekit.rtapi.config.exceptions import \
    RTAPIConfigException, RTAPIConfigNotFoundException

# Plugin exceptions
from machinekit.rtapi.plugin import RTAPIPluginException


# RTAPI Flavor exceptions
class RTAPIFlavorKernelException(RuntimeError):
    """
    Raised by flavor sanity checks when kernel doesn't match requested
    flavor
    """
    pass

