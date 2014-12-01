import logging
from machinekit.masterd import DaemonException

# For export
from keas.pbstate.meta import ProtobufState

class Service(object):
    """
    Base class for a request/reply service.  The `server` and `client`
    constructors return server and client instances of the class.

    Subclasses must override the `transport_type` attribute with its
    `transport.Socket` or other subclass, and override the
    `request_type` and `reply_type` attributes with corresponding
    subclasses of the `Request` and `Reply` classes below.

    For client instances, convenience methods wrapping request
    inputs/outputs may be useful for users.
    """
    transport_type = None
    request_type = None
    reply_type = None
    is_control_service = False
    callback_category = ''

    log = logging.getLogger(__name__)

    def __init__(self, service_role, callbacks={}):
        self.service_role = service_role
        self.callbacks = callbacks

        # Pass callbacks to reply type
        self.reply_type.callbacks.update(callbacks)

    @classmethod
    def client(cls):
        """Return client instance of the service"""
        return cls('client')

    @classmethod
    def server(cls, callbacks = None):
        """Return server instance of the service"""
        return cls('server', callbacks)

    @property
    def transport(self):
        """Lazily instantiate the service's transport object at first access"""
        if not hasattr(self, '_transport'):
            self._transport = self.transport_type(self.service_role)
        return self._transport

    def stream_handler_callback(self, request_serial):
        """Return serialized reply from serialized request."""
        # Unpickle the request object
        request = self.request_type.deserialize(request_serial)

        # Build, pickle and return the reply object
        reply = self.reply_type.build_from_request(request)
        reply_serial = reply.serialize()
        return reply_serial

    def close_callback(self):
        """
        Return a callback to be run at the server's close.
        """
        def cb():
            """Close callback; do nothing"""
            self.log.debug("Called empty close_callback")

    def add_handler_to_loop(self, loop):
        """
        Add transport poller to worker loop with payload callback
        """
        # Server instance method
        assert self.service_role == 'server'

        return self.transport.add_handler_to_loop(
            loop, self.stream_handler_callback)

    def stop_handler(self):
        """
        Stop service handler
        """
        self.transport.stop_handler()

    #  Client instance methods
    def send_request(self, request):
        """
        Sends a serialized client request object to the server, and
        receives, deserializes and returns the reply object.
        """
        # Client instance method
        assert self.service_role == 'client'

        # Pickle request object and send to service
        request_serial = request.serialize()
        self.transport.send(request_serial)

        # Receive reply, unpickle and return
        reply_serial = self.transport.recv()
        return self.reply_type.deserialize(reply_serial)

    def request(self, **kwargs):
        """
        Creates a client request object with given keys and values.
        """
        # Client instance method
        assert self.service_role == 'client'

        return self.request_type(**kwargs)

    def __str__(self):
        """
        Printable representation of service
        """
        return "%s(%s)" % (self.__class__.__name__, self.service_role)


class ControlService(Service):
    """
    The Control service class server instance is specially handled
    from outside, since it needs access to the worker and its handlers
    and loop.
    """
    is_control_service = True

    def set_worker(self, worker):
        """Interface to register worker for use by control requests"""
        self.worker = worker
        # Hack to make worker and server available in Reply objects
        self.reply_type.worker = worker
        self.reply_type.server = self


class SigHandlerService(ControlService):
    """
    The signal handler server instance is specially handled from
    outside, both as the ControlService is, and to provide a way to
    unblock signals from the `signalfd` when forking child processes.
    """
    def unblock_signals(self):
        """
        A method to pass outside for unblocking signals to child processes
        """
        pass


    def set_worker(self, worker):
        """Interface to register worker for use by control requests"""
        super(SigHandlerService, self).set_worker(worker)
        self.worker.unblock_signals_callback = self.unblock_signals


class Message(object):
    """
    Base class for Protocol Buffer-encoded request and reply messages.

    This subclass provides the basic message object type and
    serialize/deserialize routines for shipping between client and
    server.

    Service subclasses do their main work in the `Request` and `Reply`
    subclasses, which define message attributes and methods for
    processing requests and generating replies.  Most other Service
    subclasses are only needed for simple configuration or wrappers.

    Subclasses must be tied to their Python protobuf classes through
    the `keas.pbstate` package.  In a `Request` or `Reply` subclass,
    do the following to enable storing class attributes directly to
    the protobuf object:

        from keas.pbstate.meta import ProtobufState
        from masterd.proto import foo_pb2 

        class FooRequest(Request):
            __metaclass__ = ProtobufState
            protobuf_type = foo_pb2.FooRequest

    In the `Reply` subclass, define a class method constructor
    `build_from_request(request)` that performs request processing and
    creates the needed `Reply` subclass instance.
    """
    log = logging.getLogger(__name__)
    callbacks = {}

    def __init__(self,**kwargs):
        """
        This basic constructor returns a message object with given
        keys and values.
        """
        for (attr, val) in kwargs.items():
            setattr(self, attr, val)


    def serialize(self):
        """Serialize the message object"""
        pb_serial, _ = self.__getstate__()
        return pb_serial


    @classmethod
    def deserialize(cls, pb_serial):
        """Construct a message object from serialized protobuf bytes"""
        request = cls()
        # __setstate__ needs a tuple w/dict in second element
        request.__setstate__((pb_serial, {}))
        return request


    def stop_worker(self):
        """Initiate the worker shutdown process"""
        self.worker.loop_shutdown_process()


class Request(Message):
    """
    Represents a service reply message.  See the `Message` class
    documentation for details.
    """
    @property
    def method_name(self):
        """
        Default method for generating Reply object method name from a
        Request: Assume the Type attribute is an enum; return the
        lowercase of its Name appended to 'request_';
        e.g. 'request_doit'.
        """
        return "request_%s" % self.protobuf_type.Type.Name(self.type).lower()

        
class Reply(Message):
    """
    Represents a service reply message.  See the `Message` class
    documentation for details.
    """
    @classmethod
    def build_from_request(cls, request):
        """
        Default method for dispatching a Reply object method given an
        incoming Request: get the method_name from the Request and
        call the Reply method of that name.
        """
        reply = cls()
        method_name = request.method_name
        if hasattr(reply, method_name):
            method = getattr(reply, method_name)
            method(request)
            return reply
        else:
            raise DaemonException("Method not implemented:  '%s'", method_name)

