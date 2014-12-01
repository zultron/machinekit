from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from machinekit.masterd.loop import current, PollTimer
from machinekit.masterd.worker import Worker
import signal

class test_078_masterd_service(FixtureTestCase):

    def test_07810_init(self):
        """07810 masterd coprocess:  init()"""

        # Setup:  set up TestService object
        from files.test_07x_masterd_service import *
        self.fix(
            c = TestApp(),
            loop = current(),
            ta_class = TestApp,
            pt_class = PollTimer,
            )

        # Test:  attributes
        assert_equal(self.c.state, self.ta_class.PENDING)
        assert_is_instance(self.c.poll_timer, self.pt_class)
        assert_equal(self.c.loop, self.loop)


    @mock.patch('subprocess.Popen')
    def test_07820_mock_start(self, Popen):
        """07820 masterd coprocess:  mock start()"""

        # Setup: Coprocess instance w/patched timer; patched Popen;
        # mock Worker; start()
        w = mock.Mock()
        self.c.poll_timer = mock.Mock()
        self.c.start(w)

        # Test:  Popen() called; timer set(); attributes
        assert_equal(self.c.worker, w)
        Popen.assert_called_with(self.c.args,
                                 executable = self.c.executable,
                                 preexec_fn = self.c.popen_preexec_fn)
        assert_equal(self.c.state, self.ta_class.STARTED)
        self.c.poll_timer.set.assert_called_with()

    def test_07830_mock_status_poller_setup(self):
        """07830 masterd coprocess:  mock status_poller() setup"""

        # Setup:  Patches
        # timer
        self.c.poll_timer = mock.Mock()
        self.c.poll_timer.cancel = mock.Mock(
            side_effect=Exception("timer cancelled"))
        self.c.poll_timer.reset = mock.Mock(
            side_effect=Exception("timer reset"))
        self.c.poll_timer.update = mock.Mock(
            side_effect=Exception("timer updated"))
        # proc
        self.c.proc = mock.Mock()
        self.c.proc.pid = 18
        self.c.proc.poll = mock.Mock(
            side_effect=Exception("proc.poll() called"))
        # loop
        self.c.loop = mock.Mock()
        self.c.loop.add_callback = mock.Mock(
            side_effect=Exception("loop.add_callback() called"))

        # Setup: Coprocess instance w/patched Worker
        assert_is_instance(self.c.worker, mock.Mock)


    @mock.patch('os.killpg')
    def test_07831_mock_status_poller_term(self, killpg):
        """07831 masterd coprocess:  mock status_poller() first TERM"""

        print("Test1:  proc still running; state=STARTED; reset timer & return")
        # Setup:  proc.poll() returns None:  proc still running
        self.c.proc.poll = mock.Mock(return_value = None)
        # Setup:  state = STARTED
        assert_equal(self.c.state, self.ta_class.STARTED)
        # Setup:  will call poll_timer.reset()
        self.c.poll_timer.reset = mock.Mock()
        # Setup:  will not call killpg()
        killpg.side_effect = Exception("killpg called")
        # Do it
        self.c.status_poller()
        # Test:  Come back later:  Timer reset() & return
        self.c.poll_timer.reset.assert_called_with()
        # Reset:  timer reset() shouldn't be called
        self.c.poll_timer.reset = mock.Mock(
            side_effect=Exception("timer reset"))


    @mock.patch('os.killpg')
    def test_07832_mock_status_poller_sigchld(self, killpg):
        """07832 masterd coprocess:  mock status_poller() spurious SIGCHLD"""

        print("Test2:  proc still running; SIGCHLD; do nothing")
        # Setup:  proc running and state = TERMINATING
        assert_is_none(self.c.proc.poll())
        self.c.state = self.ta_class.TERMINATING
        killpg.side_effect = Exception("killpg called")
        # Do it: from_signal_handler and still running, do nothing
        self.c.check_sigchld()
        # Test:  state != EXITED: some other proc exited
        assert_equal(self.c.state, self.ta_class.TERMINATING)
        assert_false(self.c.exited)


    @mock.patch('os.killpg')
    def test_07833_mock_status_poller_kill(self, killpg):
        """07833 masterd coprocess:  mock status_poller() kill proc"""

        print("Test3:  proc still running after term; kill")
        # Setup: proc still running when state = TERMINATING and
        # called from timer
        assert_is_none(self.c.proc.poll())
        assert_equal(self.c.state, self.ta_class.TERMINATING)
        # Setup:  will call poll_timer.cancel()
        self.c.poll_timer.cancel = mock.Mock()
        # Setup:  will call loop.add_callback()
        self.c.loop.add_callback = mock.Mock()
        # Do it
        self.c.status_poller()
        # Test: proc forcefully killed: timer canceled; state =
        # EXITED; process group killed
        self.c.poll_timer.cancel.assert_called_with()
        assert_equal(self.c.state, self.ta_class.EXITED)
        assert_true(self.c.exited)
        killpg.assert_called_with(18, signal.SIGKILL)
        # schedule_worker_check() sends control back to worker
        self.c.loop.add_callback.assert_called_with(
            self.c.worker.loop_shutdown_process)
        # Reset:  timer reset() shouldn't be called
        self.c.poll_timer.cancel = mock.Mock(
            side_effect=Exception("timer canceled"))
        # Reset:  loop.add_callback() shouldn't be called
        self.c.loop.add_callback = mock.Mock(
            side_effect=Exception("loop.add_callback() called"))


    @mock.patch('os.killpg')
    def test_07834_mock_status_poller_exit(self, killpg):
        """07834 masterd coprocess:  mock status_poller() exit cleanup"""

        print("Test4:  proc exited, state = TERMINATING")
        # Setup: process exited: proc.poll() returns int; state =
        # TERMINATING; reset loop and timer
        self.c.proc.poll = mock.Mock(return_value = 42)
        self.c.state = self.ta_class.TERMINATING
        # Setup:  will call poll_timer.cancel()
        self.c.poll_timer.cancel = mock.Mock()
        # Setup:  will call loop.add_callback()
        self.c.loop.add_callback = mock.Mock()
        # Setup:  will not call killpg()
        killpg.side_effect = Exception("killpg called")
        # Do it
        self.c.status_poller()
        # Test:  state = EXITED
        assert_equal(self.c.state, self.ta_class.EXITED)
        assert_true(self.c.exited)
        # Test:  timer cancel()led
        self.c.poll_timer.cancel.assert_called_with()
        # Test:  control sent back to worker
        self.c.loop.add_callback.assert_called_with(
            self.c.worker.loop_shutdown_process)
        # Reset:  timer reset() shouldn't be called
        self.c.poll_timer.cancel = mock.Mock(
            side_effect=Exception("timer canceled"))
        # Reset:  loop.add_callback() shouldn't be called
        self.c.loop.add_callback = mock.Mock(
            side_effect=Exception("loop.add_callback() called"))


    @mock.patch('os.killpg')
    def test_07835_mock_status_poller_post_exit(self, killpg):
        """07835 masterd coprocess:  mock status_poller() post-exit cleanup"""

        print("Test5:  proc exited, state = EXITED")
        # Setup: process exited: state = EXITED
        assert_equal(self.c.state, self.ta_class.EXITED)
        killpg.side_effect = Exception("killpg called")
        # Do it
        self.c.status_poller()
        # Test: nothing happened; no exceptions means nothing was
        # touched that shouldn't have been.


    @mock.patch('os.killpg')
    def test_07840_mock_init_termination_exited(self, killpg):
        """07840 masterd coprocess:  mock init_termination() proc exited"""

        print("Test1:  proc already exited")
        self.c.proc.poll = mock.Mock(return_value = 86)
        self.c.loop.add_callback = mock.Mock() # schedule_worker_check()
        killpg.side_effect = Exception("killpg called")
        # Do it
        self.c.init_termination()
        # Test:  state=EXITED; add_callback() called
        assert_equal(self.c.state, self.ta_class.EXITED)
        self.c.loop.add_callback.assert_called_with(
            self.c.worker.loop_shutdown_process)
        # Reset
        self.c.loop.add_callback = mock.Mock(
            side_effect=Exception("loop.add_callback() called"))


    @mock.patch('os.killpg')
    def test_07841_mock_init_termination_running(self, killpg):
        """07841 masterd coprocess:  mock init_termination() proc running"""

        print("Test2:  proc running, state=RUNNING:  "
              "send SIGTERM & update timer")
        self.c.proc.poll = mock.Mock(return_value = None)
        self.c.state = self.ta_class.STARTED
        # Setup:  will call poll_timer.cancel() and update()
        self.c.poll_timer.cancel = mock.Mock()
        self.c.poll_timer.reset = mock.Mock()
        self.c.poll_timer.update = mock.Mock()
        # Do it
        self.c.init_termination()
        # Test:  state=TERMINATING; timer canceled; 
        assert_equal(self.c.state, self.ta_class.TERMINATING)
        # Test:  timer update()ed
        self.c.poll_timer.update.assert_called_with(
            seconds=self.c.kill_timer_seconds)
        # Test:  killpg() called
        killpg.assert_called_with(18, signal.SIGTERM)
        # Reset
        self.c.loop.add_callback = mock.Mock(
            side_effect=Exception("loop.add_callback() called"))
        self.c.poll_timer.cancel = mock.Mock(
            side_effect=Exception("timer canceled"))
        self.c.poll_timer.cancel = mock.Mock(
            side_effect=Exception("timer reset"))
        self.c.poll_timer.update = mock.Mock(
            side_effect=Exception("timer updated"))


