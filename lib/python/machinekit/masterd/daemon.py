import logging, os

class Daemon(object):

    log = logging.getLogger(__name__)

    def daemonize(self):
        """
        Fork off and detach from controlling terminal
        """
        REDIRECT_TO = os.devnull
        try:
            pid = os.fork()
        except OSError as e:
            self.log.error("Unable to fork process: %s [%d]" %
                           (e.strerror, e.errno))
            raise e

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


