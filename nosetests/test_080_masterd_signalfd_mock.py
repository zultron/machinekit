from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from machinekit.masterd.service.signal_fd import \
    SignalFD, SignalFDRequest, SignalFDReply, SignalFDService
from machinekit.masterd import loop
import os, signal, signalfd

class test_080_masterd_signalfd_mock(FixtureTestCase):

    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_08010_init(self, loop):
        """08010 masterd signalfd mock:  init()"""

        # Setup:  set up SignalFDService object
        self.fix(
            s = SignalFDService.server(),
            worker = mock.Mock(),
            loop = loop,
            )

        # Setup:  copy mocked workers around
        self.s.set_worker(self.worker)

        # Test:  attributes
        assert_equal(self.s.service_role, 'server')
        # Test:  patched loop
        assert_is_instance(self.s.loop, mock.Mock)
        #   (this test's side-effect is to instantiate the transport)
        assert_is_instance(self.s.transport.loop, mock.Mock)

    @mock.patch('signalfd.create_signalfd')
    def test_08020_signalfd_transport_fd(self, create_signalfd):
        """08020 masterd signalfd mock:  transport fd method"""

        # Setup:  patch signalfd.create_signalfd()
        create_signalfd.return_value = 10
        # Do it
        fd = self.s.transport.fd
        # Test:  fd value; create_signalfd() called
        assert_equal(fd, 10)
        create_signalfd.assert_called_with(self.s.transport.sigs)
        # Test:  ensure some signals in list
        for sig in ['TERM', 'CHLD', 'HUP', 'INT', 'PIPE', 'QUIT', 'STOP']:
            assert_in(getattr(signal, 'SIG%s' % sig), self.s.transport.sigs)
        
    @mock.patch('signalfd.read_signalfd')
    def test_08021_signalfd_transport_add_handler_to_loop(self, read_signalfd):
        """08021 masterd signalfd mock:  transport read_signalfd()"""

        ###  Test add_handler_to_loop() calls loop.add_handler()
        ###  with the fd and callback
        # Setup:  patched loop.add_handler()
        assert_is_instance(self.s.transport.loop, mock.Mock)
        # Setup:  signalfd patched to 10
        assert_equal(self.s.transport.fd, 10)
        # Do it
        self.s.add_handler_to_loop()
        # Test:  loop.add_handler() called
        self.s.loop.add_handler.assert_called_once
        # Test:  loop.add_handler() called with args
        #   Extract transport_stream_callback from args
        tsc = self.s.loop.add_handler.mock_calls[0][1][1]
        self.s.loop.add_handler.assert_called_with(
            10, # transport.fd patched in previous test
            tsc, self.s.loop.READ)

        # Save transport_stream_callback() for later tests
        self.fix(transport_stream_callback = [tsc])  # don't bind method


    @mock.patch('signalfd.sigprocmask')
    def test_08022_signalfd_unblock_signals(self, sigprocmask):
        """08022 masterd signalfd mock:  unblock_signals()"""

        # Setup:  patch signalfd.sigprocmask()
        assert_is_instance(signalfd.sigprocmask, mock.Mock)
        # Setup:  the fd must have been set up already
        assert_in(signal.SIGTERM, getattr(self.s.transport, 'sigs', []))
        # Do it
        self.s.unblock_signals()
        # Test:  sigprocmask() called
        sigprocmask.assert_called_with(signalfd.SIG_UNBLOCK,
                                       self.s.transport.sigs)


    def test_08030_signalfd_request_method_name(self):
        """08030 masterd signalfd mock:  Request.method_name()"""

        # SIGTERM replied to with 'shutdown' method
        r = self.s.request_type.deserialize(signal.SIGTERM)
        assert_equal(r.method_name, 'request_shutdown')

        # SIGCHLD replied to with 'worker_sigchld_callback' method
        r = self.s.request_type.deserialize(signal.SIGCHLD)
        assert_equal(r.method_name, 'request_worker_sigchld_callback')

        # SIGHUP ignored (replied to with 'ignore' method)
        r = self.s.request_type.deserialize(signal.SIGHUP)
        # Test:  method_name property
        assert_equal(r.method_name, 'request_ignore')


    @mock.patch(
        'machinekit.masterd.service.signal_fd.SignalFDReply.request_ignore')
    @mock.patch('signalfd.read_signalfd')
    def test_08040_signalfd_mock_signal_actions(self, read_signalfd,
                                                request_ignore):
        """08040 masterd signalfd mock:  signal->action tests"""

        tsc = self.transport_stream_callback[0]

        ### Overall setup
        # Setup:  patched worker
        assert_is_instance(self.s.worker, mock.Mock)
        assert_is_instance(self.s.reply_type.worker, mock.Mock)
        # Setup:  patch signalfd.read_signalfd()
        assert_is_instance(signalfd.read_signalfd, mock.Mock)
        # Setup:  fake fd number
        fake_fd = 13

        ###  Test transport_stream_callback() shuts down worker on SIGTERM
        # Setup:  fake read_signalfd() returns signal.SIGTERM
        read_signalfd.return_value = mock.Mock(ssi_signo=signal.SIGTERM)
        # Do it:  call transport_stream_callback()
        tsc(fake_fd,1)
        # Test:  signalfd.read_signalfd() called
        read_signalfd.assert_called_with(fake_fd)
        # Test:  worker.loop_shutdown_process() called
        self.worker.loop_shutdown_process.assert_called_with()

        ###  Test transport_stream_callback() calls worker cb on SIGCHLD
        # Setup:  fake read_signalfd() returns signal.SIGCHLD
        read_signalfd.return_value = mock.Mock(ssi_signo=signal.SIGCHLD)
        # Do it:  call transport_stream_callback()
        tsc(fake_fd,1)
        # Test:  worker.sigchld_callback() called
        self.worker.sigchld_callback.assert_called_with()

        ###  Test transport_stream_callback() does nothing on SIGHUP
        # Setup:  fake read_signalfd() returns signal.SIGHUP
        read_signalfd.return_value = mock.Mock(ssi_signo=signal.SIGHUP)
        # Do it:  call transport_stream_callback()
        tsc(fake_fd,1)
        # Test:  worker.sigchld_callback() called
        request_ignore.assert_called_once()

