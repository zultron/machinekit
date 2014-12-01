from . import Transport

import zmq
from zmq.eventloop import zmqstream
context = zmq.Context()

class ZMQSocket(Transport):
    """
    A service's ZMQ socket at minimum has a method (bind or connect),
    a type (REP, REQ, PUB, SUB, etc.), and a URL.  This class
    generates a socket for the service in either a server or client
    role.

    This class is intended to be subclassed with a _socket_params dict
    to configure the above attributes.
    """

    # Subclasses must override _socket_params
    _socket_params = dict(
        url                     = None,             # 'tcp://127.0.0.1:9300'
        method                  = None,             # 'bind', 'connect'
        socket_type             = None,             # 'REQ_REP', etc.
        subscribe               = None,             # b''
        )

    _translations = dict(
        #                         server                client
        method = dict(
            bind                = (zmq.Socket.bind,     zmq.Socket.connect),
            connect             = (zmq.Socket.connect,  zmq.Socket.bind),
            ),
        detach = dict(
            bind                = (zmq.Socket.unbind,     zmq.Socket.disconnect),
            connect             = (zmq.Socket.disconnect, zmq.Socket.unbind),
            ),
        socket_type = dict(
            REQ_REP             = (zmq.REP,             zmq.REQ),
            PUB_SUB             = (zmq.SUB,             zmq.PUB),
            PAIR                = (zmq.PAIR,            zmq.PAIR),
            # DEALER/ROUTER?
            ),
        )

    # The 'linger' sock option, used in closing
    linger = 1
    # Receive timeout in milliseconds
    receive_timeout_ms = None  # e.g. 1000

    # Reverse-map machine-readable int to human-readable string values
    _type_map = dict([(getattr(zmq,i), i)
                      for i in ('REP', 'REQ', 'SUB', 'PUB', 'PAIR')])
    _method_map = dict([(getattr(zmq.Socket,i), i) for i in ('connect','bind')])

    # Socket cache
    _socket_cache = {}

    @classmethod
    def _lookup(cls, service_role, attr, default=None, trans_attr=None):
        """Internal helper for _translations table lookups"""
        if trans_attr is None: trans_attr = attr
        attr_key = cls._socket_params[attr] # e.g. 'bind' or 'REQ_REP'
        value_index = service_role == 'client' # index in (server, client) tuple
        return cls._translations[trans_attr][attr_key][value_index]

    @property
    def socket_type(self):
        """Internal convenience property"""
        return self._lookup(self.service_role, 'socket_type')

    @property
    def service_url(self):
        """ZMQ socket URL"""
        return self._socket_params['url']

    @property
    def socket_method(self):
        """Internal convenience property"""
        return self._lookup(self.service_role, 'method')

    @property
    def socket_detach(self):
        """Internal convenience property"""
        return self._lookup(self.service_role, 'method', trans_attr='detach')

    @property
    def socket_subscribe(self):
        """ZMQ socket option zmq.SUBSCRIBE value"""
        # Default (None) is subscribe to everything
        return self._socket_params.get('subscribe',None)

    @property
    def socket(self):
        """
        Lazily init socket with attributes derived from protobuf and
        class.
        """
        if self.service_role == 'server':
            key = self.service_url
        else:
            key = self
        if not self._socket_cache.has_key(key):
            # Create socket with type zmq.REQ, zmq.REP, etc.
            socket = self._socket_cache[key] = \
                context.socket(self.socket_type)
            # Call socket.bind or socket.connect
            self.socket_method(socket, self.service_url)
            # Set socket options, if applicable
            if self._socket_params.get('subscribe', None) is not None:
                socket.setsockopt(zmq.SUBSCRIBE,
                                  self._socket_params['socket_subscribe'])
            if self.receive_timeout_ms is not None:
                socket.RCVTIMEO = self.receive_timeout_ms
        return self._socket_cache[key]

    def __str__(self):
        """Printable representation of socket"""
        return '%s: (%s/%s)@%s' % \
            (self.__class__.__name__,
             self._type_map[self.socket_type],
             self._method_map[self.socket_method],
             self.service_url)

    def add_handler_to_loop(self, loop, stream_handler_callback):
        """
        Add a stream from the server's socket to the worker loop and
        set server's callbacks.
        """
        def transport_callback(stream, zmq_msg):
            """Give request message received from zmq stream to
            payload callback; send payload reply back to zmq stream"""
            request_serial = zmq_msg[0]  # REQ_REP socket msg in [0]
            reply_serial = stream_handler_callback(request_serial)
            stream.send(reply_serial)

        # Attach socket poller to loop and add callback
        self.handler = zmqstream.ZMQStream(self.socket, loop)
        self.handler.on_recv_stream(transport_callback)

        # Return handler for control purposes
        return self.handler

    def stop_handler(self):
        """
        Shut down service handler
        """
        self.handler.flush()
        self.handler.on_recv(None)


    def detach(self):
        """unbind or disconnect from a socket; probably never needed"""
        self.socket.close(linger=1)
        self.socket_detach(self.socket, self.service_url)

    def send(self, request_serial):
        """Send bytes over ZMQ socket"""
        self.socket.send(request_serial)

    def recv(self):
        """Receive bytes from ZMQ socket"""
        reply_serial = self.socket.recv()
        return reply_serial

    @property
    def closed(self):
        return self.socket.closed

