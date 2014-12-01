from . import *
from machinekit.masterd.transport import Transport

import re, signal, signalfd, os
import logging

class SignalFD(Transport):
    """
    The signalfd stream
    """

    log = logging.getLogger(__name__)

    # regex for selecting signals from module attributes
    signal_re = re.compile(r'SIG[^_]')

    @property
    def all_signal_names(self):
        """List of all signal names defined in `signal` module"""
        return sorted([
                s for s in dir(signal) if self.signal_re.match(s)])

    @property
    def fd(self):
        """
        Lazily init file descriptor for all signals
        """
        if not hasattr(self, '_sfd'):
            # Handle *all* signals
            self.handled_signals = self.all_signal_names

            self.sigs = sorted(
                dict([(getattr(signal, s),1) \
                          for s in self.handled_signals]).keys())
            self._sfd = signalfd.create_signalfd(self.sigs)

        return self._sfd


    def unblock_signals(self):
        """
        Unblock all signals blocked by signalfd.

        This is passed to the worker so that signals can be unblocked
        for coprocesses.

        http://lwn.net/Articles/415684/
        """
        self.log.debug("    Unblocking signals")
        signalfd.sigprocmask(signalfd.SIG_UNBLOCK, self.sigs)


    def add_handler_to_loop(self, loop, exception_handler):
        """
        Create signalfd and add to loop with exception handler callback
        """
        def transport_stream_callback(fd, whatsis):
            sig = signalfd.read_signalfd(fd)
            exception_handler(sig.ssi_signo)

        loop.add_handler(
            self.fd, transport_stream_callback, loop.READ)
        
    def stop_handler(self):
        '''
        Unblock signals and close signalfd
        '''
        self.unblock_signals()
        os.close(self.fd)

    def __str__(self):
        return "%s:  fd %d" % (self.__class__.__name__, self._sfd)


class SignalFDRequest(Request):
    """SignalFD signal"""
    shutdown_signals = [2, 9, 14, 15]
    child_term_signal = 17

    log = logging.getLogger(__name__)

    @classmethod
    def deserialize(cls, sig_num):
        """Return SignalFDReply with sig_num set"""
        request = cls(sig_num = sig_num)
        return request

    @property
    def method_name(self):
        # Always do the same thing
        return 'request_send_signal'

    def __str__(self):
        return "<%s sig_num=%d>" % (self.__class__.__name__, self.sig_num)

class SignalFDReply(Reply):

    log = logging.getLogger(__name__)

    def serialize(self):
        pass

    def request_send_signal(self, signal_request):
        self.log.debug("  Passing signal %d to signal callback",
                       signal_request.sig_num)
        self.callbacks['signal'](signal_request.sig_num)


class SignalFDService(SigHandlerService):
    transport_type = SignalFD
    request_type = SignalFDRequest
    reply_type = SignalFDReply
    log = logging.getLogger(__name__)

    callback_category = 'signal'

    def unblock_signals(self):
        """
        A method to pass outside for unblocking signals to child processes
        """
        self.transport.unblock_signals()
