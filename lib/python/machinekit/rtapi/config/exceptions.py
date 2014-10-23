class RTAPIConfigException(RuntimeError):
    """
    Fatal errors raised by config store system
    """
    pass

class RTAPIConfigNotFoundException(RuntimeError):
    """
    Raised by config store system when config item not found
    """
    pass

