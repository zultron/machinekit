class Transport(object):
    """
    Abstract class representing a transport for service requests and
    replies

    Subclasses must implement `add_handler_to_loop()` and `recv()`,
    and may implement `stop_handler()` and `send()` as applicable.
    """

    def __init__(self, service_role):
        """
        Creates the Transport object, assigning it a 'server' or
        'client' service role.
        """
        assert service_role in ('client', 'server')
        self.service_role = service_role

        self.handler = None


    # Server methods
    def add_handler_to_loop(self, loop, stream_handler_callback):
        """
        Add a stream from the server's socket to the worker loop and
        set server's callbacks.
        """
        raise TransportException(
            "Transport subclasses must implement add_handler_to_loop()")


    def stop_handler(self):
        """
        Shut down service handler
        """
        # Not necessary to implement
        pass


    # Client methods
    def recv(self):
        """Receive transport output and return serialized message"""
        raise TransportException(
            "Transport subclasses must implement recv()")


    def send(self, request_serial):
        """Send serialized request bytes over transport"""
        # Not necessary to implement
        pass


class TransportException(Exception):
    """Raised by Transport subclasses"""
    pass
