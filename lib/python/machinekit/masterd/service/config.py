from . import *
import config_pb2 as pb_msg
from machinekit.masterd.transport.zmq_socket import ZMQSocket

class ConfigSocket(ZMQSocket):
    """Config service runs on port 9301"""
    _socket_params = dict(
        service_type        = 'config',
        url                 = 'tcp://127.0.0.1:9301',
        method              = 'bind',
        socket_type         = 'REQ_REP',
        )


class ConfigRequest(Request):
    """Config service request object"""
    __metaclass__ = ProtobufState
    protobuf_type = pb_msg.ConfigRequest

    log = logging.getLogger(__name__)


class ConfigReply(Reply):
    """Config service reply object"""
    __metaclass__ = ProtobufState
    protobuf_type = pb_msg.ConfigReply

    log = logging.getLogger(__name__)

    def request_get(self, request):
        self.log.debug("Request 'get( name=%s )'", request.name)
        self.type = 10         # GET_RESULT
        self.name = request.name
        self.value_string = "fake_val"
        
    def request_set(self, request):
        self.log.debug("Request 'set( name=%s )'", request.name)
        self.type = 20         # SET_RESULT_OK
        self.name = request.name


class ConfigService(Service):
    """Config service"""
    transport_type = ConfigSocket
    request_type = ConfigRequest
    reply_type = ConfigReply
    log = logging.getLogger(__name__)

    # Client methods
    def get(self, name=None):
        """Get config item value"""
        return self.send_request(self.request(type=10, name=name)).value_string

    def set(self, name=None, **kwargs):
        """Set config item value"""
        self.send_request(self.request(type=20, name=name, **kwargs))

