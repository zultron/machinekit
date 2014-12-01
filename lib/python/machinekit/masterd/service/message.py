from . import *
import machinekit.masterd.proto.masterd_pb2 as masterd
from machinekit.masterd.transport.zmq_socket import ZMQSocket

class MessageSocket(ZMQSocket):
    _socket_params = dict(
        service_type        = 'message',
        url                 = 'tcp://127.0.0.1:9303',
        method              = 'bind',
        socket_type         = 'REQ_REP',
        )

class MessagRequest(Request):
    __metaclass__ = ProtobufState
    protobuf_type = masterd.MessagRequest

    log = logging.getLogger(__name__)


class MessagReply(Reply):
    __metaclass__ = ProtobufState
    protobuf_type = masterd.MessagReply

    log = logging.getLogger(__name__)


class MessageService(Service):
    transport_type = MessageSocket
    request_type = MessageRequest
    reply_type = MessageReply
    log = logging.getLogger(__name__)

    callback_category = 'message'
