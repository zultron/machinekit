from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from machinekit.masterd.loop import \
    IOLoopCoroutine, IOLoopCoroutineError, event_callback

class TestFSM(IOLoopCoroutine):
    state_machine = mock.Mock()


class test_073_masterd_loop(FixtureTestCase):

    def test_07310_loop_init(self):
        """07310 masterd coroutine:   init loop"""

        # Setup:  init loop and mock object
        self.fix(
            )


    def test_07320_coroutine_event_callback(self):
        """07320 masterd coroutine:   @event_callback decorator"""

        # Setup:  method with event_callback decorator
        meth_mock = mock.Mock(return_value=42)
        @event_callback
        def meth(obj, *args, **kwargs):
            return meth_mock(*args, **kwargs)

        # Setup: fake IOLoopCoroutine object
        obj = mock.Mock()
        # obj._event_callback_nested = mock.MagicMock(return_value=False)
        del obj._event_callback_nested
        obj.stopped = False

        # Setup:  call method
        res = meth(obj,1,2,foo=3,bar=4)

        # Test: function return value passed back from event_callback
        assert_equal(res, 42)

        # Test: called with args
        meth_mock.assert_called_with(1,2,foo=3,bar=4)

        # Test:  coroutine callback added to loop
        obj.loop.add_callback.assert_called_with(
            obj.state_machine_obj.next)

        # Setup:  reset mock
        meth_mock.reset_mock()

        # Setup:  2nd method with nesting
        meth_mock2 = mock.Mock(return_value=13)
        @event_callback
        def meth2(obj, *args, **kwargs):
            meth(obj)
            return meth_mock2(
                88,  nested=getattr(obj, '_event_callback_nested', False),
                *args, **kwargs)

        # Setup: clean fake IOLoopCoroutine object
        obj = mock.Mock()
        # obj._event_callback_nested = mock.MagicMock(return_value=False)
        del obj._event_callback_nested
        obj.stopped = False

        # Test:  calling clear_timeout raises exception
        obj.clear_timeout.side_effect = Exception(
            '@event_callback should not clear_timeout()')

        # Setup:  call 2nd method
        res = meth2(obj)

        # Test:  check nesting and calls
        meth_mock2.assert_called_with(88, nested=True)
        meth_mock.assert_called_with()
        assert_false(obj._event_callback_nested)

        # Test:  callback added exactly once
        obj.loop.add_callback.assert_called_once_with(
            obj.state_machine_obj.next)


    def test_07322_coroutine_event_callback_args(self):
        """07322 masterd coroutine:   @event_callback() with args"""

        # Setup:  method with clear_timeout
        meth_mock = mock.Mock(return_value=42)
        @event_callback(clear_timeout=True)
        def meth(obj, *args, **kwargs):
            return meth_mock(*args, **kwargs)

        # Setup: fake IOLoopCoroutine object
        obj = mock.Mock()
        # obj._event_callback_nested = mock.MagicMock(return_value=False)
        del obj._event_callback_nested
        obj.stopped = False

        # Setup:  call method
        res = meth(obj,1,2,foo=3,bar=4)

        # Test: function return value passed back from event_callback
        obj.clear_timeout.assert_called_once_with()

        # Setup:  method with False clear_timeout
        @event_callback(clear_timeout=False)
        def meth(obj, *args, **kwargs):
            return meth_mock(*args, **kwargs)

        # Test:  calling clear_timeout raises exception
        obj.clear_timeout.side_effect = Exception(
            '@event_callback should not clear_timeout()')

        # Setup:  call method
        res = meth(obj,1,2,foo=3,bar=4)

        # Test:  callback was added (sanity)
        obj.loop.add_callback.assert_called_with(
            obj.state_machine_obj.next)

    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07330_coroutine_new_state_machine_obj(self, ioloop):
        """07330 masterd coroutine:   new_state_machine_obj()"""

        # Round 1:  install state machine
        #
        # Setup:  New state machine; mocked loop
        c = TestFSM()
        assert_is_instance(c.loop, mock.Mock)
        c.new_state_machine_obj()

        # Test: attributes
        assert_false(c.shutdown_requested)
        assert_false(c.restart_requested)
        assert_false(c.stopped)
        assert_is_none(c.timeout_ref)
        assert_is_not_none(getattr(c, 'state_machine_obj', None))

        # Test: callback added to loop
        c.loop.add_callback.assert_called_with(c.state_machine_obj.next)

        # Round 2:  install shutdown state machine
        #
        # Setup:  New state machine with shutdown_state_machine; mocked loop
        c = TestFSM()
        c.shutdown_state_machine = mock.Mock()
        assert_is_instance(c.loop, mock.Mock)
        c.new_state_machine_obj(shutdown=True)

        # Test:  shutdown_state_machine() called
        c.shutdown_state_machine.assert_called_once_with()

        # Test: attributes
        assert_true(c.shutdown_requested)
        assert_is_instance(c.state_machine_obj, mock.Mock)

        # Test: callback added to loop
        c.loop.add_callback.assert_called_with(c.state_machine_obj.next)

        
    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07331_coroutine_init(self, ioloop):
        """07331 masterd coroutine:   __init__()"""

        # Setup:  init object
        c = TestFSM(session_loop=True)

        # Test state_machine_obj
        assert_is_not_none(getattr(c, 'state_machine_obj', None))

        # Test session_loop
        assert_true(c.session_loop)


    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07333_coroutine_timeouts(self, ioloop):
        """07333 masterd coroutine:   timeouts"""

        # Setup: timeout not none; reset loop; fake timeout and call
        # timeout()
        c = TestFSM()
        c.loop.reset_mock()
        assert_false(c.loop.timeout.called) # Sanity
        c.timeout_ref = True
        c.timeout(5)

        # Test: check add_timeout called; get callback
        assert_true(c.loop.add_timeout.called)
        print tuple(c.loop.add_timeout.call_args)
        (delta, timeout_cb,), _ = c.loop.add_timeout.call_args

        # Test:  remove_timeout() called and timeout set
        assert_true(c.loop.remove_timeout.called)
        assert_is_not_none(c.timeout_ref)
        assert_not_equal(c.timeout_ref, True)  # From above

        # Setup:  clear mock and run timeout_cb
        c.state_machine.reset_mock()
        assert_false(c.state_machine.called)  # Sanity
        timeout_cb()

        # Test:  timeout_expired set and state_machine run
        assert_true(c.timeout_expired)
        c.state_machine_obj.next.assert_called_once_with()
        
        # Setup:  clear mock; save timeout_ref and run clear_timeout()
        c.loop.reset_mock()
        assert_false(c.loop.remove_timeout.called)  # Sanity
        assert_is_not_none(c.timeout_ref)  # Sanity
        timeout_ref_saved = c.timeout_ref
        c.clear_timeout()

        # Test:  timeout_ref cleared and remove_timeout() called
        assert_is_none(c.timeout_ref)
        c.loop.remove_timeout.assert_called_once_with(timeout_ref_saved)



    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07335_coroutine_shutdown(self, ioloop):
        """07335 masterd coroutine:   shutdown()"""

        # Round 1:  regular shutdown()
        #
        # Shutdown prep runs normally, with state flags set and
        # coroutine callback added

        # Setup:  init fsm; save coroutine; reset loop
        c = TestFSM()
        cor = c.state_machine_obj
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called) # Sanity

        # Setup:  call shutdown()
        c.shutdown('Testing shutdown')

        # Sanity:  Mock objects
        assert_is_instance(c.loop, mock.Mock)

        # Test attributes
        assert_true(c.shutdown_requested)
        assert_false(hasattr(c,'shutdown_state_machine'))
        # Test:  coroutine wasn't swapped out for shutdown coroutine
        assert_equal(cor, c.state_machine_obj)

        # Test:  decorator added callback to loop
        c.loop.add_callback.assert_called_once_with(c.state_machine_obj.next)

        # Round 2:  duplicate shutdown()
        #
        # Shutdown bails immediately; callback added once again

        # Setup:  reset loop obj
        c.loop.reset_mock()

        # Sanity checks:  mock object clear; shutdown_requested True
        assert_false(c.loop.add_callback.called)
        assert_true(c.shutdown_requested)

        # Setup:  call shutdown() a second time
        c.shutdown('Testing shutdown again')

        # Test:  callback re-added to loop
        assert_true(c.loop.add_callback.called)

        # Round 3:  Shutdown with 'shutdown_state_machine'
        #
        # Reset object and add 'shutdown_state_machine'

        # Setup:  reset attributes and add 'shutdown_state_machine' method
        c = TestFSM()
        c.shutdown_state_machine = mock.Mock()

        # Setup:  run shutdown()
        c.shutdown()

        # Test:  state machine obj from shutdown_state_machine
        assert_is_instance(c.state_machine_obj, mock.Mock)


    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07336_coroutine_signal(self, ioloop):
        """07336 masterd coroutine:   signal()"""

        # Setup: init object, reset mock, set restart_requested
        c = TestFSM()
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called) # Sanity
        assert_false(c.shutdown_requested) # Sanity
        c.restart_requested = True
        import signal
        c.signal(signal.SIGTERM)

        # Test:  restart_requested reset
        assert_false(c.restart_requested)

        # Test:  callback added
        c.loop.add_callback.assert_called_once_with(c.state_machine_obj.next)

        # Test:  shutdown called
        assert_true(c.shutdown_requested)


    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07340_coroutine_restart(self, ioloop):
        """07340 masterd coroutine:   restart()"""

        # Round 1:  regular restart()
        #
        # Shutdown prep runs normally, with state flags set and
        # coroutine callback added

        # Setup:  init FSM; clear mock loop; restart
        c = TestFSM()
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called)   # Sanity
        assert_false(c.shutdown_requested)  # Sanity
        c.restart('Testing restart')

        # Test attributes
        assert_true(c.restart_requested)
        assert_true(c.shutdown_requested)  # shutdown() called

        # Test:  decorator added callback to loop
        c.loop.add_callback.assert_called_once_with(c.state_machine_obj.next)

        # Round 2:  duplicate restart()
        #
        # Restart bails immediately; callback added once again

        # Setup:  reset loop obj and call restart() again
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called) # Sanity
        assert_true(c.restart_requested)  # Sanity
        c.shutdown = mock.Mock()
        msg = 'Testing restart again'
        c.restart(msg)

        # Test:  restart_requested is True and shutdown() not called
        assert_true(c.restart_requested)
        assert_false(c.shutdown.called)

        # Test:  callback re-added to loop
        assert_true(c.loop.add_callback.called)

    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07342_coroutine_exit(self, ioloop):
        """07342 masterd coroutine:   exit()"""

        # Setup: new FSM; reset loop
        c = TestFSM(session_loop=False)
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called) # Sanity

        # Setup:  exit()
        c.exit()

        # Test:  callback added
        c.loop.stop.assert_called_once_with()
        

    @mock.patch('zmq.eventloop.ioloop.IOLoop')
    def test_07344_coroutine_exit_session(self, ioloop):
        """07344 masterd coroutine:   exit_session()"""

        # Round 1:  session loop w/session_loop
        #
        # Check that exit_session() when session_loop is set
        # starts a new state_machine

        # Setup: timeout not none; reset loop and set session_loop;
        # save state machine object; set fake timeout_ref
        c = TestFSM(session_loop=True)
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called) # Sanity
        c.timeout_ref = 42
        c.state_machine.reset_mock()
        assert_false(c.loop.state_machine.called) # Sanity

        # Setup:  exit_session
        c.exit_session()

        # Test:  state machine replaced
        c.state_machine.assert_called_once_with()

        # Test:  timeout cleared; callback added
        assert_is_none(c.timeout_ref)
        c.loop.add_callback.assert_called_once_with(c.state_machine_obj.next)


        # Round 2:  session loop w/restart_requested
        #
        # Check that exit_session() when restart_requested is set
        # starts a new state_machine

        # Setup: reset loop and set restart_requested
        c = TestFSM(session_loop=False)
        c.restart_requested = True
        c.state_machine.reset_mock()
        assert_false(c.loop.state_machine.called) # Sanity

        # Setup:  exit_session
        c.exit_session()

        # Test:  state machine replaced
        c.state_machine.assert_called_once_with()


        # Round 3:  no session loop w/stop_loop_on_shutdown set
        #
        # Check that IOLoop is stopped when stop_loop_on_shutdown is
        # set

        # Setup: timeout not none; reset loop and set session_loop;
        # save state machine object; set fake timeout_ref
        c = TestFSM(session_loop=False)
        c.state_machine.reset_mock()
        assert_false(c.loop.state_machine.called) # Sanity
        c.loop.reset_mock()
        assert_false(c.loop.add_callback.called)  # Sanity
        c.stop_loop_on_shutdown = True

        # Setup:  exit_session
        c.exit_session()

        # Test:  loop.stop callback added
        c.loop.stop.assert_called_once_with()
        

    
    def test_07350_coroutine_no_mock(self):
        """07350 masterd coroutine:   coroutines with real IOLoop"""

        def timedelta(seconds):
            import datetime
            return datetime.timedelta(seconds=seconds)

        # Setup:  a simple FSM
        c = TestFSM()
        LOCKED=1
        UNLOCKED=2
        # events
        @event_callback
        def push(self):
            if self.state == UNLOCKED:
                self.log.debug("push:  transitioning from UNLOCKED to LOCKED")
                self.state = LOCKED
        TestFSM.push = push
        @event_callback
        def coin(self):
            if self.state == LOCKED:
                self.log.debug("coin:  transitioning from LOCKED to UNLOCKED")
                self.state = UNLOCKED
        TestFSM.coin = coin
        def turnstile(self):
            timebase = self.loop.time()
            def logtime(): self.log.debug(
                "time: %1.3fs" % (self.loop.time() - timebase))
            # Don't loop session
            self.stop_loop_on_shutdown = True
            # Start in locked state
            self.state = LOCKED

            # State loop
            while not self.shutdown_requested:
                logtime()
                if self.timeout_expired:
                    self.log.debug("Got here on timeout")
                    self.hit_timeout = True
                if self.state == LOCKED:
                    self.log.debug("In LOCKED state")
                elif self.state == UNLOCKED:
                    self.log.debug("In UNLOCKED state")
                else:
                    self.log.error("Unknown state '%s'", self.state)
                    break
                yield self.timeout(1)

            logtime()
            self.log.info("Stopping turnstile")
            yield self.exit_session()
        TestFSM.state_machine = turnstile
        c.new_state_machine_obj()

        def sig(obj, signame):
            import signal
            signum = getattr(signal,'SIG%s' % signame)
            def callsig(): obj.signal(signum)
            return callsig

        # RUN 1:  go through some states, end with SIGTERM
        #
        # Be sure we shutdown
        c.loop.add_timeout(timedelta(10), c.exit)

        # Pretend a series of events
        c.loop.add_timeout(timedelta(0.1), c.push)
        c.loop.add_timeout(timedelta(0.2), c.push)
        c.loop.add_timeout(timedelta(0.3), c.coin)
        c.loop.add_timeout(timedelta(0.35), sig(c, 'HUP'))
        c.loop.add_timeout(timedelta(0.4), c.push)
        c.loop.add_timeout(timedelta(0.45), sig(c, 'TERM'))
        c.start()

        # Test:  stopped set; timeout unset
        assert_true(c.stopped)
        assert_is_none(c.timeout_ref)

        # RUN 2:  go through some states, end with shutdown
        #
        # Be sure we shutdown
        c.loop.add_timeout(timedelta(10), c.exit)

        # Pretend a series of events
        c.loop.add_timeout(timedelta(0.1), c.push)
        c.loop.add_timeout(timedelta(0.2), c.coin)
        c.loop.add_timeout(timedelta(0.3), c.push)
        c.loop.add_timeout(timedelta(1.4), c.shutdown)
        c.new_state_machine_obj()
        c.start()

        # Test:  detected timeout
        assert_true(getattr(c,'hit_timeout',False))
