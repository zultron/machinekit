from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from machinekit.masterd import loop
import zmq, os

class test_076_masterd_service(FixtureTestCase):

    def test_07610_init(self):
        """07610 masterd service:  init()"""

        # Build the python protobuf test service module
        os.system("protoc \
	    --python_out=%(filesdir)s \
	    --proto_path=%(filesdir)s \
	    %(filesdir)s/test_07x_masterd_service.proto" %
                  dict(filesdir = self.filesdir))

        # Setup:  set up TestService object
        from files.test_07x_masterd_service import *
        self.fix(
            s = TestService.server(),
            c = TestService.client(),
            loop = loop.current(),
            ts = TestService,
            tt = TestSocket,
            trq = TestRequest,
            trp = TestReply,
            )

        # Test:  attributes
        assert_equal(self.s.service_role, 'server')
        assert_equal(self.c.service_role, 'client')
        assert_equal(self.s.loop, self.loop)

    def test_07620_transport_property(self):
        """07620 masterd service:  transport property"""

        # Setup:  init transport
        st = self.s.transport
        ct = self.c.transport

        # Test:  object types
        assert_is_instance(st, self.tt)
        assert_is_instance(ct, self.tt)

    @mock.patch("zmq.eventloop.zmqstream.ZMQStream")
    def test_07630_add_handler_to_loop(self, stream):
        """07630 masterd service:  add_handler_to_loop()"""

        # Setup:  patch ZMQStream and call method
        assert_equal(stream, zmq.eventloop.zmqstream.ZMQStream)
        self.s.add_handler_to_loop()

        # Test: object calls ZMQStream object (through transport
        # method)
        stream.assert_called_with(self.s.transport.socket, self.loop)

        # Test:  adding client to loop raises exception
        assert_raises(AssertionError,self.c.add_handler_to_loop)

    def test_07631_stop_handler(self):
        """07631 masterd service:  stop_handler()"""

        # Setup:  patched server from last step; call method
        assert_is_instance(self.s.transport.handler, mock.Mock)
        self.s.stop_handler()

        # Test: object calls ZMQStream object (through transport
        # method)
        self.s.transport.handler.flush.assert_called_with()
        self.s.transport.handler.on_recv.assert_called_with(None)
        self.s.transport.handler.close.assert_called_with(linger=1)

    @mock.patch('files.test_07x_masterd_service.TestSocket')
    def test_07632_client_send_request(self, TestSocket):
        """07632 masterd service:  client send_request()"""

        # Setup: patched transport w/patched recv() method and canned
        # reply
        reply_in = self.trp(type=10)
        TestSocket.recv = mock.Mock(return_value=reply_in.serialize())

        # Setup: service with patched transport
        c = self.ts.client()
        c._transport = TestSocket()
        assert_is_instance(c.transport, mock.Mock)

        # Setup: call send_request() with canned request
        request = self.trq(type=10)
        reply_out = c.send_request(request)

        # Test: object calls ZMQStream send/recv methods through transport
        c.transport.send.assert_called_with(request.serialize())
        c.transport.recv.assert_called_with()
        
        # Test: received sane reply
        assert_is_instance(reply_out, self.trp)
        assert_equal(reply_out.type, 10)
