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

class RTAPIFlavorPrivilegeException(RuntimeError):
    """
    Raised by flavor sanity checks when user has insufficient
    privileges to access real-time environment
    """
    pass

class RTAPIFlavorULimitException(RuntimeError):
    """
    Raised by flavor sanity checks when system resource limits are
    insufficient
    """
    pass
