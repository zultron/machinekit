from machinekit.masterd.service import *
from machinekit.masterd.coprocess import Coprocess
import files.test_07x_masterd_service_pb2 as masterd
from machinekit.masterd.transport.zmq_socket import ZMQSocket

import os
curdir = os.path.dirname(__file__)

class TestSocket(ZMQSocket):
    """Test service runs on port 9302"""
    _socket_params = dict(
        service_type        = 'test',
        url                 = 'tcp://127.0.0.1:9302',
        method              = 'bind',
        socket_type         = 'REQ_REP',
        )
    receive_timeout_ms = 1000

class TestRequest(Request):
    """Test service request object"""
    __metaclass__ = ProtobufState
    protobuf_type = masterd.TestRequest
    log = logging.getLogger(__name__)

    def __str__(self):
        return ("Request: type=%s, name=%s, strvalue=%s, "
                "intvalue=%d, boolvalue=%s" % \
                    (self.type, self.name, self.strvalue, self.intvalue,
                     self.boolvalue))

class TestReply(Reply):
    """Test service reply object"""
    __metaclass__ = ProtobufState
    protobuf_type = masterd.TestReply
    log = logging.getLogger(__name__)

    def request_test1(self, request):
        self.log.info("Received request <%s>", request)
        self.type = 10 # TEST1_OK
        self.name = "test_reply"
        self.strvalue = request.strvalue + " reply"
        self.intvalue = request.intvalue - 29
        self.boolvalue = not request.boolvalue

    def __str__(self):
        return ("Reply: type=%s, name=%s, strvalue=%s, "
                "intvalue=%d, boolvalue=%s" % \
                    (self.type, self.name, self.strvalue, self.intvalue,
                     self.boolvalue))


class TestService(ControlService):
    """Test service"""
    transport_type = TestSocket
    request_type = TestRequest
    reply_type = TestReply
    log = logging.getLogger(__name__)


class TestApp(Coprocess):

    args = [ 'test_app' ]
    executable = '%s/test_07x_masterd_coprocess.sh' % curdir

    log = logging.getLogger(__name__)

