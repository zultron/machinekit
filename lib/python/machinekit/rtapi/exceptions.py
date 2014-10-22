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

# Plugin exceptions
class RTAPIPluginException(RuntimeError):
    """
    Raised by plugin system
    """
    pass

# Config store exceptions
class RTAPIConfigException(RuntimeError):
    """
    Raised by config store system
    """
    pass

class RTAPIConfigNotFoundException(RuntimeError):
    """
    Raised by config store system when config item not found
    """
    pass

