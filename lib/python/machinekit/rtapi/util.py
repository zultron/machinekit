import logging, psutil, os, sys
import compat_bindings

class RTAPIUtilRuntimeError(RuntimeError):
    """RTAPI utility runtime error"""
    pass

class Util(object):
    def __init__(self, config=None):
        if config is None:
            raise RTAPIUtilRuntimeError("Util object needs rtapi.config")
        self.log = logging.getLogger(self.__module__)
        self.config = config

    def proc_by_name(self,name):
        for p in psutil.process_iter():
            if p.name == name:
                return p

    def proc_by_cmd(self,cmd):
        """Get process PID by argv[0]"""
        # Needed to identify e.g. msgd:0, who doctored its own argv[0]
        for p in psutil.process_iter():
            if len(p.cmdline) > 0 and p.cmdline[0] == cmd:
                return p

    def insert_module(self, name, *args):
        try:
            self.compat.run_module_helper(
                "insert %s %s" % (name, ' '.join(self.config.shmdrv_opts)))
        except RuntimeError as e:
            raise RTAPIUtilRuntimeError(
                "Failed to insert %s module:\n    %s" % (name, e))

    def daemonize(self):
        """
        Fork off and detach from controlling terminal
        """
        REDIRECT_TO = os.devnull
        try:
            pid = os.fork()
        except OSError as e:
            raise RTAPIUtilRuntimeError("Unable to fork process: %s [%d]" %
                                        (e.strerror, e.errno))

        # exit the foreground process
        if pid != 0:
            self.log.info("Closing foreground process %s" %
                          os.getpid())
            os._exit(0)   # safer way for fg process to exit

        # become session leader
        os.setsid()

        # redirect stdin/out/err from/to /dev/null
        os.open("/dev/null", os.O_RDWR)
        os.dup2(0,1)
        os.dup2(0,2)

        self.log.info("Successfully forked off background process %s" %
                      os.getpid())
        return
