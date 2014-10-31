import compat_bindings

import logging

class RTAPICompatRuntimeError(RuntimeError):
    """RTAPI compat runtime error"""
    pass

class Compat(object):
    
    def __init__(self, config=None):
        if config is None:
            raise RTAPICompatRuntimeError("Compat object needs rtapi.config")
        self.log = logging.getLogger(self.__module__)
        self.config = config

    @property
    def kernel_instance_id(self):
        return compat_bindings.kernel_instance_id()

    def is_module_loaded(self,name):
        return compat_bindings.is_module_loaded(name)
