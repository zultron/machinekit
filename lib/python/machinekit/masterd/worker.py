import logging, loop

class Worker(object):
    """
    The Worker class runs services and coprocesses in a Tornado IOLoop.

    During initialization, the `Worker` adds 'servers' to the main I/O
    loop.  The loop polls each server's ZMQ socket and calls its
    handler for output during run.  The signal handler is also
    implemented as a 'server' with a file descriptor polled in the
    same way.

    The `Worker` also adds coprocesses to the loop.  During run, the
    `Coprocess` object manages its state via callbacks invoked from
    periodic timers attached to the loop, or invoked by signal
    handlers.
    """
    log = logging.getLogger(__name__)

    # Shutdown states
    STATE_RUNNING = 0
    STATE_SHUTDOWN_INIT = 1
    STATE_SHUTDOWN_COPROCESSES = 2
    STATE_SHUTDOWN_COMPLETE = 3

    def __init__(self, servers=[], coprocesses=[]):
        """
        Initialize Worker object with servers and coprocesses.
        """
        # Lists of servers and coprocesses to run
        self.servers = servers
        self.coprocesses = coprocesses
        self.loop = loop.current()
        self.unblock_signals_callback = None
        self.state = self.STATE_RUNNING

    def run(self):
        """
        Set up IOLoop with handlers for each service and run.
        """
        self.log.debug("Initializing worker loop")

        # Add servers to worker loop
        for server in self.servers:
            self.add_server(server)

        # Add coprocesses to loop
        for coprocess in self.coprocesses:
            self.add_coprocess(coprocess)

        # Start worker loop
        self.log.debug("Starting worker loop")
        self.loop.start()

        # Worker loop exited
        self.log.info("All servers stopped")


    def add_server(self, server):
        """Add server to worker loop"""

        # Control services need ref to worker
        if server.is_control_service:
            server.set_worker(self)

        # Add stream handler to loop
        server.add_handler_to_loop(self.loop)

        self.log.info("    Added service %s", server.transport)


    def add_coprocess(self, coprocess):
        """Add coprocess to worker loop"""
        coprocess.start(self)
        self.log.info("    Started %s", coprocess)
        

    def loop_shutdown_process(self):
        """
        This begins a state machine for shutting down.  Start by
        telling each coproc to shut down.  Those will schedule
        `check_coprocs()` when they exit, which will start the next
        step of stopping services.
        """
        if self.state == self.STATE_RUNNING:
            self.log.info("Entering shutdown process")
            self.state = self.STATE_SHUTDOWN_INIT

            # Set up timer to guarantee control is returned
            self.shutdown_polltimer = loop.PollTimer(
                self.loop_shutdown_process, seconds=1)
            self.shutdown_polltimer.set()

            self.log.debug("  Stopping coprocesses")
            for coprocess in reversed(self.coprocesses):
                coprocess.init_termination()
            return
            # From here, coprocesses will manage their own
            # termination; control will come back when
            # Coprocess.schedule_worker_check() is run when a coproces
            # exits
        elif self.state == self.STATE_SHUTDOWN_INIT:
            # If coprocesses not yet stopped, do nothing
            for coproc in self.coprocesses:
                if not coproc.exited:
                    self.shutdown_polltimer.reset()
                    return
            self.shutdown_polltimer.cancel()

            self.log.debug("  Coprocesses terminated; stopping servers")
            self.state = self.STATE_SHUTDOWN_COPROCESSES

            for server in reversed(self.servers):
                self.log.debug("    Shutting down %s", server)
                server.stop_handler()

            self.log.debug("  Servers terminated; stopping main loop")
            self.loop.stop()
            self.state = self.STATE_SHUTDOWN_COMPLETE
            self.log.info("Worker shutdown complete")
            return


    def sigchld_callback(self):
        """Method for external signal handlers to notify worker of
        SIGCHLD event"""
        for coproc in self.coprocesses:
            coproc.check_sigchld()


    def unblock_signals(self):
        """
        Method passed from SigHandlerService, used to unblock signals
        for a Coprocess
        """
        if self.unblock_signals_callback is not None:
            self.unblock_signals_callback()
