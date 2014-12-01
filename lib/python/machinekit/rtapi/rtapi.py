import logging, rtapi_bindings

class RTAPI(object):

    _RTAPIcommand = None

    def __init__(self, config):
        self.log = logging.getLogger(self.__module__)
        self.config = config

    @property
    def RTAPIcommand(self):
        """
        Establish an RTAPI command channel as a fixture in the class
        scope to survive class instance setup/teardown; the
        RTAPIcommand method can only be run once; a second invocation
        hoses everything
        """
        if self._RTAPIcommand is None:
            self.__class__._RTAPIcommand = \
                rtapi_bindings.RTAPIcommand(uuid=self.config.mkuuid)
        return self._RTAPIcommand

    def loadrt(self,*args):
        return self.RTAPIcommand.loadrt(*args, instance=self.config.instance)

    def delthread(self,name):
        return self.RTAPIcommand.delthread(name, instance=self.config.instance)

class RTAPIPermissionError(RuntimeError):
    """General permission error class for RTAPI"""
    pass
