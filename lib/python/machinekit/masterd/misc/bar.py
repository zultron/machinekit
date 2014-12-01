import logging, re, os, sys
import select, signal, signalfd, fcntl
import subprocess
from zmq.eventloop import ioloop
from tornado.concurrent import Future
from tornado import gen, concurrent
import datetime
import time
ioloop.install()

logging.basicConfig(level=logging.DEBUG)

class bar(object):
    
    log = logging.getLogger(__name__)

    # regex for selecting signals from module attributes
    signal_re = re.compile(r'SIG[^_]')

    @property
    def all_signal_names(self):
        """List of all signal name defined in signal module"""
        return sorted([
                s for s in dir(signal) if self.signal_re.match(s)])

    @classmethod
    def signumbers(cls, signames):
        """Translate signal name or list of names into signal number
        or list of numbers"""
        if isinstance(signames, str):
            return getattr(signal, name)
        return map(lambda s: getattr(signal, s), signames)

    def __init__(self, create_signalfd=False):
        self.handled_signals = self.all_signal_names

        if create_signalfd:
            self.sigs = [getattr(signal, s) for s in self.handled_signals]
            # self.sigs = [signal.SIGCHLD]
            # self.sfd = signalfd.create_signalfd(self.sigs,signalfd.SFD_CLOEXEC)

            signalfd.sigprocmask(signalfd.SIG_BLOCK, self.sigs)
            self.sfd = signalfd.signalfd(-1, self.sigs, signalfd.SFD_CLOEXEC)

            print "flags = %s" % fcntl.fcntl(self.sfd, fcntl.F_GETFL)
            print "SFD_CLOEXEC = %s" % signalfd.SFD_CLOEXEC
            fcntl.fcntl(self.sfd, fcntl.F_SETFL, fcntl.FD_CLOEXEC)
            print "flags = %s" % fcntl.fcntl(self.sfd, fcntl.F_GETFL)
            print "FD_CLOEXEC= %s" % fcntl.FD_CLOEXEC
            # FIXME tryclosing it
            os.close(self.sfd)

        # self.log.debug(
        #     'Capturing signals: %s' %
        #     ' '.join([str(s) for s in sorted(self.sigs)]))
        # self.log.debug("Process pid %s", os.getpid())

    def signalfd_callback(self, future):
        self.log.info("In callback; got future %s", future)
        self.log.debug("dir(future):  %s", dir(future))
        self.log.debug("future done? %s", future.done())
        self.log.debug("future running? %s", future.running())
        self.log.debug("future result %s", future.result())
        # self.log.debug("self.sig = %s", self.sig)
        # how to continue the future generator?  can we just return?
        sys.exit(1)
        # next(future)  # this doesn't work; future isn't a generator?

    def stop(self):
        self.log.info("cleaning up")
        self.loop.stop()
        self.log.info("exiting")
        sys.exit(0)

    # def stoppy(self):
    #     self.log.info("stoppy!")
    #     self.stop()

    # def add_fut(self):
    #     self.loop.add_future(self.signalfd_future(), self.signalfd_callback)

    # @gen.coroutine
    @concurrent.return_future
    def signalfd_future(self, callback):
        self.log.info("Waiting for signal")
        sig = signalfd.read_signalfd(self.sfd)
        self.log.info("Caught signal %s", sig)
        self.sig = sig
        callback(sig)
        # r = yield select.select([self.sfd], [], [])[0]
        # for s in r:
        #     sig = yield signalfd.read_signalfd(self.sfd)
        #     self.log.info("Caught signal %s", sig)
        #     self.sig = sig

    @gen.engine
    def do_future(self, callback):
        yield self.signalfd_future()
        callback()

    def handle_sig(self, foo, bar):
        sig = signalfd.read_signalfd(self.sfd)
        self.log.info("Caught signal %s, args %s %s", sig, foo, bar)
        self.sig = sig


    def run(self):
        # Get IOLoop instance for this thread
        self.poller = ioloop.ZMQPoller()

        # self.log.debug("add signalfd %d to poller", self.sfd)
        # self.poller.register(self.sfd, ioloop.IOLoop.READ)

        self.log.debug("setting up loop")
        self.loop = ioloop.IOLoop(impl=self.poller)
        self.loop.make_current()


        self.loop.add_callback(self.popen)
        self.loop.add_timeout(datetime.timedelta(seconds=10),
                              self.stop)

        # only allow 5 seconds blocking
        # self.loop.set_blocking_log_threshold(5)
        # self.loop.set_blocking_signal_threshold(4,self.stoppy)

        # self.log.debug("adding timeout")
        # self.loop.add_timeout(datetime.timedelta(seconds=10),
        #                       self.stop)
        # self.log.debug("dir(loop) %s", dir(self.loop))
        
        # self.log.debug("adding signalfd handler, fd=%d", self.sfd)
        # self.loop.add_handler(self.sfd, self.handle_sig, self.loop.READ)
        # self.log.debug("dir(loop) %s", dir(self.loop))
        
        # self.log.debug("adding future")
        # self.loop.add_future(self.do_future(self.signalfd_callback), self.signalfd_callback)
        # self.loop.add_future(self.signalfd_future(), self.signalfd_callback)
        # self.loop.add_callback(self.add_fut)
        self.log.debug("starting loop")
        # self.loop.run_sync(self.signalfd_future)
        self.loop.start()
        self.log.debug("loop stopped")

    def remove_sig_handler_and_set_session(self):
        # signalfd.create_signalfd([signal.SIG_DFL])
        self.log.info("Closing sfd %s", self.sfd)
        os.close(self.sfd)
        self.log.info("Emptying signalfd mask")
        signalfd.sigprocmask(signalfd.SIG_BLOCK, [])

        self.log.info("setting sid")
        os.setsid()
        self.log.info("Setting a lot of signals back to default")
        for s in self.sigs:
            try:
                signal.signal(s,signal.SIG_DFL)
            except Exception as e:
                self.log.error("Problem with signal %s: %s", s, e)
        signalfd.sigprocmask(signalfd.SIG_UNBLOCK, self.sigs)

    def popen(self):
        print "running popen"
        self.proc = subprocess.Popen(
            ["/tmp/machinekit/machinekit/src/daemon/fake_rtapi_app.sh"],
            close_fds=True,
            # preexec_fn=os.setsid,
            # preexec_fn=os.setpgrp,
            preexec_fn=self.remove_sig_handler_and_set_session,
            )
        time.sleep(2)
        print "terminating"
        # self.proc.terminate()
        # os.kill(self.proc.pid, signal.SIGTERM)
        os.killpg(self.proc.pid, signal.SIGTERM)
        time.sleep(1)
        print "poll %s" % self.proc.poll()



if __name__ == "__main__":
    b = bar(create_signalfd=True)
    b.run()

    # print     b.signalfd_future()
