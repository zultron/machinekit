# Copyright (c) 2012 - 2013 John Morris <john@zultron.com>

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Python 'co-routines' [1] are a simple, clean and intuitive way to
# implement FSMs. This module implements an FSM with a co-routines
# that manages state transitions, and events that may cause state
# transitions are managed by a ZMQ ioloop[2] context. The mechanisms
# for trasferring control back and forth between the state machine and
# ioloop are:

# - Relinquishing control to the ioloop via 'yield' in the co-routine

# - Returning control back to the co-routine by passing its
#   'next()' method as a callback to:

#   - 'add_timeout()': The FSM may relinquish and regain control after a
#     period

#   - 'add_callback()': External events pass control to FSM for
#     servicing via subclass methods

# - Methods wrapped with the `@event_callback` decorator are used as
#   ioloop event handler callbacks. The methods update FSM internal
#   state, and the decorator schedules return of control with
#   `add_callback()`.

# [1]: http://eli.thegreenplace.net/2009/08/29/co-routines-as-an-alternative-to-state-machines
# [2]: http://zeromq.github.io/pyzmq/api/generated/zmq.eventloop.ioloop.html

class IOLoopCoroutineError(RuntimeError):
    pass

def event_callback(*args, **kwargs):
    '''
    Run the decorated method followed by `loop.add_callback()` to
    schedule control to be passed back to the FSM on the next loop.

    In case one event_callback calls another, the callback will only be
    added once.

    The 'clear_timeout=True' argument will clear any pending timeout.
    '''
    # Extract decorator arguments
    clear_timeout=kwargs.get('clear_timeout', False)

    # Double-wrapper function
    from functools import wraps
    def wrap_wrap(func):
        @wraps(func)
        def wrap(obj, *args, **kwargs):
            # The callback should run once in the top-level decorator;
            # keep track of possible nesting
            nested = getattr(obj, '_event_callback_nested', False)
            if not nested: obj._event_callback_nested = True

            # Run the function
            res = func(obj, *args, **kwargs)

            # If not nested and FSM not stopped, add coroutine's callback
            if not nested and not obj.stopped:
                obj.loop.add_callback(obj.state_machine_obj.next)
                obj._event_callback_nested = False

            # clear timeout if requested
            if clear_timeout: obj.clear_timeout()

            return res
        return wrap

    # Return the appropriate wrapper
    if len(args) > 0 and callable(args[0]):
        # '@event_callback' form:  unwrap outer layer
        return wrap_wrap(args[0])
    else:
        # '@event_callback(...)' form
        return wrap_wrap


class IOLoopCoroutine(object):
    '''
    Mealy FSM base class where a co-routine handles state-changing
    events in a ZMQ IOLoop

    Subclasses must override the `state_machine()` method to implement
    the FSM. Subclasses may implement a `shutdown_state_machine() FSM
    method that will be swapped in place of the `state_machine()` FSM
    when `shutdown()` is called.
    '''
    import logging
    log = logging.getLogger(__name__)

    stop_loop_on_shutdown = False
    '''
    If the `stop_loop_on_shutdown` attribute is `True`, after shutting
    down, stop the IOLoop
    '''

    STATE_INIT = 0
    STATE_EXITED = 1

    def __init__(self, session_loop=False):
        '''
        Initialize state machine
        '''
        # Set state to STATE_INIT
        self.state = self.STATE_INIT

        # After session shutdown, restart if session_loop is True
        self.session_loop = session_loop

        # IOLoop singleton instance for coordinating control
        from zmq.eventloop import ioloop
        ioloop.install()
        self.loop = ioloop.IOLoop.current()
        # Opaque timeout object and flag indicating whether control
        # was returned from the timeout or an intervening event
        self.timeout_ref = None
        self.timeout_expired = False

        # Init state machine object
        self.new_state_machine_obj()


    def new_state_machine_obj(self, shutdown=False, restart=False):
        '''
        Instantiate state machine and install as current
        '''
        # Init state
        self.shutdown_requested = shutdown
        self.restart_requested = restart
        self.stopped = False
        # Reset timeout and ensure old state machine won't be restarted
        self.clear_timeout()

        # Init state machine object
        if shutdown and hasattr(self, 'shutdown_state_machine'):
            self.state_machine_obj = self.shutdown_state_machine()
        else:
            self.state_machine_obj = self.state_machine()

        # Add to loop
        self.loop.add_callback(self.state_machine_obj.next)


    def state_machine(self):
        '''
        The main state machine method

        A subclass must override this method to implement its FSM.
        '''
        err = "Subclasses must override 'state_machine' method"
        self.log.error(err)
        raise IOLoopCoroutineError(err)

    def start(self):
        '''
        Start the IOLoop

        This should only happen once ever in the managing FSM.
        '''
        self.loop.start()


    def clear_timeout(self):
        '''
        Clear any timeout
        '''
        if self.timeout_ref is not None:
            self.loop.remove_timeout(self.timeout_ref)
            self.timeout_ref = None
            self.log.debug("      Cleared timeout")


    def timeout(self, seconds, callback):
        '''
        Create a timeout in the current loop with the given callback;
        return a reference to the timeout
        '''
        import datetime
        return self.loop.add_timeout(
            datetime.timedelta(seconds=seconds), callback)

    def set_timeout(self, seconds):
        '''
        Sets a timeout, after which the FSM may 'yield' control

        If no intervening event occurs first, the FSM will regain
        control after the specified number of seconds, upon which
        'timeout_expired' will be set.
        '''
        self.clear_timeout()
        self.timeout_expired = False
        def timeout_cb():
            self.timeout_expired = True
            return self.state_machine_obj.next()
        self.timeout_ref = self.timeout(seconds, timeout_cb)
        self.log.debug("      Set %s-second timeout", seconds)
        

    @event_callback
    def shutdown(self, msg=None):
        '''
        'shutdown' callback for external control

        Set 'shutdown_requested' flag; detect repeated requests

        The '@event_callback' decorator schedules control to be
        returned to the FSM on the next loop
        '''
        if msg is None:  msg = "External shutdown request"
        if self.shutdown_requested:
            self.log.debug("%s:  Shutdown already in progress" % msg)
        else:
            # If there's a separate shutdown state machine coroutine,
            # set it up as new state machine
            if hasattr(self, 'shutdown_state_machine'):
                self.log.info("%s:  Starting shutdown state machine" % msg)
                self.new_state_machine_obj(shutdown=True,
                                           restart=self.restart_requested)
            else:
                self.shutdown_requested = True
                self.log.info("%s:  Starting shutdown sequence" % msg)


    @event_callback
    def signal(self, in_signal):
        '''
        Handle a signal
        '''
        import signal
        if in_signal in (signal.SIGTERM, signal.SIGINT):
            self.restart_requested = False
            self.shutdown('Received signal %s' % in_signal)
        else:
            self.log.debug('Received unhandled signal %d', in_signal)


    @event_callback
    def restart(self, msg=None):
        '''
        'restart' callback for external control

        Set 'restart_requested' flag and call 'shutdown'. When
        `state_machine()` yields to `exit_session()`, a new state
        machine will be started.
        '''
        if msg is None:  msg = "External restart request"
        if self.restart_requested:
            self.log.debug("%s:  Restart already in progress" % msg)
        else:
            self.restart_requested = True
            self.shutdown(msg)
    

    def exit_session(self):
        '''
        Take care of either exiting or restarting the state machine

        Especially those subclasses using a session model should call
        this when ending a session. If the `session_loop` attribute is
        `True`, the state machine will be restarted; otherwise, if the
        `stop_loop_on_shutdown` attribute is `True`, the IOLoop will
        be stopped. Otherwise, the state machine will simply stop.
        '''
        # Be sure stale timeout will not revive the state machine
        self.clear_timeout()  # stop timer

        if self.session_loop or self.restart_requested:
            self.log.info("Restarting state machine")
            self.new_state_machine_obj()
        elif self.stop_loop_on_shutdown:
            self.exit()

        self.stopped = True   # disable event_callback

    def exit(self):
        '''
        Schedule the main IOLoop to be stopped at the next loop
        '''
        self.log.info("Stopping state machine")
        self.state = self.STATE_EXITED
        self.loop.stop()
