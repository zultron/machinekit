import logging, subprocess, os, signal, signalfd
from machinekit.masterd.loop import IOLoopCoroutine, event_callback

class Coprocess(IOLoopCoroutine):
    '''
    FSM to keep track of a coprocess life cycle

    This base class must be subclassed.

    The state machine first forks the coprocess.  Subclasses must
    override `args` and may override `executable` (default `args[0]`).

    While the coprocess runs, it is periodically polled every
    `poll_timer_seconds` (default 5) with the `poll()` method, until
    it exits or a terminating event occurs.

    If the coprocess is still running during the shutdown process, an
    attempt to terminate it gracefully by calling the `term()` method
    (default `SIGTERM`) and waiting up to `kill_timer_seconds`; these
    may be overridden by subclasses.

    If the coprocess is still running after the grace period, it is
    forcefully stopped with the `kill()` method.

    At this point the state machine exits.

    External agents may request process termination through the
    `shutdown()` method, and signal handlers should pass signals
    through the `signal()` method.

    The `exited` property is `True` when the coprocess has exited.
    '''

    args = None
    executable = None
    poll_timer_seconds = 5
    kill_timer_seconds = 5
    handled_signals = [ signal.SIGTERM, signal.SIGINT, ]

    log = logging.getLogger(__name__)

    PENDING = 0
    STARTED = 1
    TERMINATING = 2
    EXITED = 3

    def __init__(self):
        '''
        Init coprocess state machine
        '''
        assert self.args is not None  # Sanity

        if self.executable is None:  self.executable = self.args[0]
        self.log.info("Initializing coprocess, command:  '%s'",
                      ' '.join(self.args))
        super(Coprocess, self).__init__()

        self.received_term_signal = False
        self.state = self.PENDING

    def fork(self):
        '''
        Fork the coprocess
        '''
        self.log.info("Forking coprocess %s", self)

        def popen_preexec_fn():
            """Set up environment for child process"""
            # Be sure the parent process's signalfd doesn't block signals
            # to the child.  See http://lwn.net/Articles/415684/
            signalfd.sigprocmask(signalfd.SIG_UNBLOCK, self.handled_signals)
            # Start new session for process
            os.setsid()

        self.proc = subprocess.Popen(
            self.args,
            executable=self.executable,
            preexec_fn=popen_preexec_fn,
            )


    def poll(self):
        '''
        Poll the coprocess, returning None if still ok or an integer
        exit value if not

        Subprocesses may override with more intelligent checks.
        '''
        return self.proc.poll()


    def term(self):
        '''
        Send SIGTERM to coprocess
        '''
        self.log.debug("    sending SIGTERM to %s", self)
        os.killpg(self.proc.pid, signal.SIGKILL)


    def kill(self):
        '''
        Send SIGKILL to coprocess
        '''
        self.log.debug("    sending SIGKILL to %s", self)
        os.killpg(self.proc.pid, signal.SIGKILL)


    def state_machine(self):
        '''
        The coprocess management state machine

        - Fork a coprocess

        - While coprocess runs, schedule return of control every two
        seconds to poll for coprocess exit (default: catch SIGCHLD
        events and poll coprocess); when the coprocess exits or a
        shutdown request or terminating signal is received, break the
        loop

        - If the coprocess is still running, send SIGTERM and set 5-second
        timeout

        - Run a loop, stopping either after the 5-second timeout or the
        coprocess exits (SIGCHLD would return control)

        - If the coprocess is still running, send SIGKILL

        - Stop state machine
        '''
        # Fork coprocess  FIXME:  handle exceptions
        self.fork()
        self.state = self.STARTED

        # Poll process and signals
        while self.poll() is None and not self.shutdown_requested:
            yield self.timeout(self.poll_timer_seconds)

        # At this point, some event occurred to trigger shutdown.
        self.state = self.TERMINATING

        # If coprocess is not running, we received a SIGCHLD. If it is
        # running, we got a kill signal or shutdown request, or an
        # overridden poll() function detected a problem, so try to kill
        # gracefully: send SIGTERM and wait up to five seconds before
        # killing forcefully
        if self.proc.poll():
            self.log.info("Attempting graceful shutdown")
            self.term()
            yield self.timeout(self.kill_timer_seconds)

        # Wait for either termination grace period timeout or
        # coprocess to exit
        while not self.timeout_expired and self.proc.poll():
            yield

        # At this point, the process has exited, or it's hung.
        if self.proc.poll():
            self.log.info("Failed to terminate; killing %s", self)
            self.kill()
        self.state = self.EXITED

        # Exit state machine
        yield self.complete_session()


    @property
    def exited(self):
        """Return boolean indicating whether process has exited.  For
        external status queries."""
        return self.state == self.EXITED


    def __str__(self):
        """
        Printable representation of coprocess
        """
        pidstr = " (pid %d)" % self.proc.pid if self.state >= self.STARTED \
            else ""
        return "Coprocess %s%s" % (self.__class__.__name__, pidstr)
