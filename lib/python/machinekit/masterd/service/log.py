from . import *
import machinekit.masterd.proto.masterd_pb2 as masterd
from machinekit.masterd.transport.zmq_socket import ZMQSocket

class LogSocket(Socket):
    """Config service runs on port 9302"""
    _socket_params = dict(
        service_type        = 'log',
        url                 = 'tcp://127.0.0.1:9302',
        method              = 'bind',
        socket_type         = 'REQ_REP',
        )

class LogRequest(Request):
    """Config service request object"""
    __metaclass__ = ProtobufState
    protobuf_type = masterd.LogRequest

    log = logging.getLogger(__name__)


class LogReply(Reply):
    """Config service reply object"""
    __metaclass__ = ProtobufState
    protobuf_type = masterd.LogReply

    log = logging.getLogger(__name__)


class LogService(Service):
    """Config service"""
    transport_type = LogSocket
    request_type = LogRequest
    reply_type = LogReply
    log = logging.getLogger(__name__)
