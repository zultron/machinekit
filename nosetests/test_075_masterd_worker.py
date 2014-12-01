from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from files.test_07x_masterd_service import TestSocket
from machinekit.masterd.worker import Worker

import threading, zmq

class test_075_masterd_worker(FixtureTestCase):

    def test_07520_worker_init(self):
        """07520 masterd worker:   init worker"""

        # Setup:  fake coprocs and services
        self.fix(c1 = mock.Mock(is_control_service=True),
                 c2 = mock.Mock(is_control_service=False),
                 s1 = mock.Mock(),
                 s2 = mock.Mock(),
                 )

        # Setup:  init Worker
        self.fix(w = Worker(
                servers=[self.s1, self.s2],
                coprocesses=[self.c1, self.c2],
                ))

        # Tests:  check attributes
        assert_equal(self.w.state, self.w.STATE_RUNNING)
        assert_equal(self.w.loop, self.w.loop.current())
        assert_equal(self.w.servers, [self.s1, self.s2])
        assert_equal(self.w.coprocesses, [self.c1, self.c2])
        assert_is_none(self.w.unblock_signals_callback)


    def test_07521_worker_mock_run(self):
        """07521 masterd worker:   run() with mock loop"""

        # Setup mock loop and run
        self.w.loop = mock.Mock()
        self.w.run()

        # Test: coproc start() method called
        self.c1.start.assert_called_with(self.w)
        self.c2.start.assert_called_with(self.w)

        # Test: control service set_worker() method called
        self.s1.set_worker.assert_called_with(self.w)

        # Test: service add_handler_to_loop() mothed called
        self.s1.add_handler_to_loop.assert_called_with()
        self.s2.add_handler_to_loop.assert_called_with()

        # Test:  loop start() method called
        self.w.loop.start.assert_called_with()


    @mock.patch('machinekit.masterd.loop.PollTimer')
    def test_07530_worker_mock_shutdown_running(self, PollTimer):
        """07530 masterd worker:   mock shutdown from RUNNING"""

        # Setup: Worker in initial STATE_RUNNING state; mock PollTimer;
        # run loop_shutdown_process()
        assert_equal(self.w.state, self.w.STATE_RUNNING)
        PollTimer.return_value = mock.Mock()
        self.w.loop_shutdown_process()

        # Test:  in STATE_SHUTDOWN_INIT
        assert_equal(self.w.state, self.w.STATE_SHUTDOWN_INIT)

        # Test:  PollTimer use
        PollTimer.assert_called_with(self.w.loop_shutdown_process, seconds=1)
        self.w.shutdown_polltimer.set.assert_called_with()

        # Test:  Coprocess access
        self.c1.init_termination.assert_called_with()
        self.c2.init_termination.assert_called_with()

    @mock.patch('machinekit.masterd.loop.PollTimer')
    def test_07531_worker_mock_shutdown_init(self, PollTimer):
        """07531 masterd worker:   mock shutdown from SHUTDOWN_INIT"""

        # Setup:  a coproc not exited
        self.c1.exited = True
        self.c2.exited = False
        self.w.loop_shutdown_process()

        # Test:  state hasn't changed; timer reset
        assert_equal(self.w.state, self.w.STATE_SHUTDOWN_INIT)
        self.w.shutdown_polltimer.reset.assert_called_with()

        # Setup:  coprocs exited
        self.c2.exited = True
        self.w.loop_shutdown_process()

        # Test: timer canceled; state updated
        self.w.shutdown_polltimer.cancel.assert_called_with()
        assert_equal(self.w.state, self.w.STATE_SHUTDOWN_COMPLETE)

        # Test:  servers stopped and loop stopped
        self.s1.stop_handler.assert_called_with()
        self.s2.stop_handler.assert_called_with()
        self.w.loop.stop.assert_called_with()


