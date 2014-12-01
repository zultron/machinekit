from . import *
import control_interface_pb2 as pb_msg
from machinekit.masterd.transport.zmq_socket import ZMQSocket

class ControlInterfaceSocket(ZMQSocket):
    """Config service runs on port 9300"""
    _socket_params = dict(
        service_type        = 'control_interface',
        url                 = 'tcp://127.0.0.1:9300',
        method              = 'bind',
        socket_type         = 'REQ_REP',
        )


class ControlInterfaceRequest(Request):
    """ControlInterface service request object"""
    __metaclass__ = ProtobufState
    protobuf_type = pb_msg.ControlInterfaceRequest

    log = logging.getLogger(__name__)


class ControlInterfaceReply(Reply):
    """ControlInterface service reply object"""
    __metaclass__ = ProtobufState
    protobuf_type = pb_msg.ControlInterfaceReply

    log = logging.getLogger(__name__)


    def request_shutdown(self, request):
        self.log.debug("  Shutdown requested:  scheduling")
        self.callbacks['shutdown']()
        self.type = self.protobuf_type.Type.Value('SHUTDOWN')
        self.result = self.protobuf_type.Shutdown.Result.Value('OK')


    def request_restart(self, request):
        self.log.debug("  Restart requested:  scheduling")
        self.callbacks['restart']()
        self.type = self.protobuf_type.Type.Value('RESTART')
        self.result = self.protobuf_type.Restart.Result.Value('OK')


    def request_status(self, request):
        self.log.debug("  Status requested:  creating status report")
        self.type = self.protobuf_type.Type.Value('STATUS')
        for handler, server in self.worker.handlers.items():
            self.handler.append(dict(
                    transport = server.transport,
                    receiving = handler.receiving(),
                    closed = handler.closed(),
                    ))

    def request_ping(self, request):
        self.log.debug("  Ping:  sending ack")
        self.type = self.protobuf_type.Type.Value('PING_ACK')

    def __str__(self):
        myname = self.__class__.__name__
        mytype = self.protobuf_type.Type.Name(self.type)
        if mytype == 'SHUTDOWN':
            myres = self.protobuf_type.Shutdown.Result.Name(
                self.shutdown.result)
        elif mytype == 'RESTART':
            myres = self.protobuf_type.Restart.Result.Name(
                self.restart.result)
        elif mytype == 'STATUS':
            myres = "(status result unimplemented)"
        return "%s %s: %s" % (myname, mytype, myres)
             

class ControlInterfaceService(ControlService):
    """ ControlInterface service"""
    transport_type = ControlInterfaceSocket
    request_type = ControlInterfaceRequest
    reply_type = ControlInterfaceReply
    log = logging.getLogger(__name__)

    callback_category = 'control'

    # Client methods
    def shutdown(self):
        """Shutdown worker loop"""
        return self.send_request(self.request(
                type=self.request_type.protobuf_type.Type.Value('SHUTDOWN')))

    def restart(self):
        """Restart worker loop"""
        return self.send_request(self.request(
                type=self.request_type.protobuf_type.Type.Value('RESTART')))

    def status(self):
        """Retrieve status of all configured services"""
        return self.send_request(self.request(
                type=self.request_type.protobuf_type.Type.Value('STATUS')))

    def ping(self):
        """Ping control interface service"""
        result = self.send_request(self.request(
                type=self.request_type.protobuf_type.Type.Value('PING')))

