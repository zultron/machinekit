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

from loop import IOLoopCoroutine, IOLoopCoroutineError, event_callback
from service.signal_fd import SignalFDService
from service.control_interface import ControlInterfaceService
from service.config import ConfigService

class SessionManager(IOLoopCoroutine):
    '''
    A FSM for managing Machinekit sessions
    '''

    stop_loop_on_shutdown = True

    STATE_INTER_SESSION = 100
    STATE_SESSION_RUNNING = 101
    STATE_SESSION_SHUTTING_DOWN = 102

    import logging
    log = logging.getLogger(__name__)

    def __init__(
        self,
        config=None,
        session_loop=True,
        persistent_services = [
            ConfigService,
            # SignalFDService,
            ControlInterfaceService,
            ],
        inter_session_services = [],
        session_services = [],
        coprocesses = [],
        **kwargs):
        '''
        Initialize the master daemon FSM
        '''
        super(SessionManager, self).__init__(
            session_loop=session_loop, **kwargs)

        # The application config
        self.config = config

        # event_callbacks dicts for use by services; add our own
        # callbacks now
        self.event_callbacks = dict(
            control = dict(
                shutdown = self.shutdown,
                restart = self.restart,
                ),
            signal = dict(
                signal = self.signal,
                ),
            config = dict(
                set_config = self.set_config,
                ),
            )

        # Services
        self.services = dict(
            persistent = self.dictlist(persistent_services),
            inter_session = self.dictlist(inter_session_services),
            session = self.dictlist(session_services),
            )
        # Session coprocesses
        self.coprocesses = self.dictlist(coprocesses)

    def dictlist(self, things):
        return [dict(name=t.__name__, cls=t) for t in things]

    def add_service(self, service):
        '''
        Add a service to the ioloop
        '''
        service_class = service['cls']
        callback_category = getattr(service_class,'callback_category','none')
        callbacks = self.event_callbacks.get(callback_category, {})
        service['inst'] = service_class.server(callbacks=callbacks)
        service['inst'].add_handler_to_loop(self.loop)
        self.log.debug("  Added service '%s', category '%s'",
                       service['name'], callback_category)
        self.log.debug("     callbacks '%s'", "', '".join(callbacks.keys()))

    def remove_service(self, service):
        '''
        Remove service from the ioloop
        '''
        service['inst'].stop_handler()
        self.log.debug("  Removed service '%s'", service['name'])

    def manage_services(self, service_category, action):
        '''
        Add, Remove or Close all services in a category
        '''
        if not self.services[service_category]:
            self.log.debug("  No services to %s in category '%s'",
                           action, service_category)
            return
        # Remove in reverse order of adding
        if action == 'add':  services = self.services[service_category]
        else: services = reversed(self.services[service_category])
        # Call add_service() or remove_service() method on each service
        for service in services:
            getattr(self,'%s_service'%action)(service)


    @event_callback(clear_timeout=True)
    def set_config(self, config):
        '''
        A callback for the 'config picker' service to pass the chosen
        config to the masterd

        The '@event_callback(clear_timeout=True)' clears any
        timeouts and passes control to the masterd FSM on the next
        loop.
        '''
        self.log.info("External request, set config to %s" % config)
        self.config = config

    def poll_config_choice(self):
        '''
        If a configuration has been chosen, set it up and return True,
        else False
        '''
        if self.config is None:
            self.log.debug("  Waiting for config choice")
            # Signal to state machine that no config was found
            return False

        self.log.info("Setting up chosen config %s" % self.config)
        return True

    def state_machine(self):
        '''
        The masterd session state machine

        The basic state flow:

        - Wait for a config to be picked; the 'config picker' service
        will call 'set_config(filename)'

        - Start up RT 

        - Start up app (linuxcncsrvr, milltask, etc. co-routines)

        - Optionally schedule periodic service activities while app
        runs

        When an external event handler executes the
        'shutdown()' callback, this FSM will be replaced in
        the loop with the 'shutdown_state_machine' FSM; see its
        documentation.
        '''
        ################### INIT STATE ##################
        self.log.info("Init server")
        # Start persistent services: signal handler, ZMQ control
        # interface, config service
        self.manage_services('persistent','add')

        # Start inter-session services:  config picker
        self.manage_services('inter_session', 'add')

        ################### INTER_SESSION STATE ##################
        # After this point, the FSM is in STATE_INTER_SESSION state
        self.state = self.STATE_INTER_SESSION

        # Loop until a configuration is picked; control will exit this
        # loop either when a configuration is picked or when shutting
        # down (and the FSM is replaced by shutdown_state_machine)
        while not self.poll_config_choice():
            # No config chosen; yield to ioloop for two seconds
            self.log.info("Waiting for configuration selection")
            yield self.set_timeout(10)

        # Stop inter-session services:  config picker
        self.manage_services('inter_session', 'remove')

        # Start session services:  any?
        self.manage_services('session','add')

        # Initialize the RT environment
        self.start_rt()

        # Start coprocesses:  rtapi_app, linuxcncsvr, task, io, etc.
        self.start_coprocesses()
        
        ################### SESSION_RUNNING STATE ##################
        # After this point, the FSM is in STATE_SESSION_RUNNING state
        self.state = self.STATE_SESSION_RUNNING

        while not self.shutdown_requested:
            self.log.debug("App running")
            yield self.set_timeout(10)

        # Catch final next() call
        yield True

        # After this, the shutdown_state_machine will be run, and the
        # new state will be SESSION_SHUTTING_DOWN

    def shutdown_state_machine(self):
        '''
        Shutdown the session

        When `shutdown()` is called, whether from an event handler or
        indirectly through a signal, this FSM will be swapped in place
        of the 'state_machine' FSM. A `shutdown()` event may happen at
        any time. The below routines should be prepared to shutdown in
        cases where the app or RT were only partially started or not
        started at all.

        - Stop app (co-routines)

        - Stop RT

        - Stop IOLoop

        (For a proper Machinekit session flow, instead of stopping the
        IOLoop, the 'state_machine' FSM should be restarted.)
        '''
        ################### SESSION_SHUTTING_DOWN STATE ##################
        # At this point, the FSM is in STATE_SESSION_SHUTTING_DOWN state
        self.state = self.STATE_SESSION_SHUTTING_DOWN

        # Stop the session
        self.log.info("Stopping app")
        self.log.info("Stopping RT")

        # Stop session services
        self.manage_services('session','remove')

        # Exit or restart state machine
        yield self.exit_session()

        # Catch final next() call
        yield True

        # After this, exit_session() will run, either restarting the
        # session and setting state INTER_SESSION, or else exiting and
        # setting state EXITED.

    def exit_session(self):
        '''
        Remove persistent services before calling
        `IOLoopCoroutine.exit_session()`
        '''

        # Remove persistent services: signal handler, ZMQ control
        # interface, config service
        self.manage_services('persistent','remove')

        # Now exit session as usual
        super(SessionManager, self).exit_session()
