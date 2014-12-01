from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises
import mock

from files.test_07x_masterd_service import TestSocket

import threading, zmq

class test_071_masterd_transport_offline(FixtureTestCase):

    def test_07110_prep(self):
        """07110 masterd transport:  prepare for tests"""

        # Get transport instances and save pristine socket params
        self.fix(ts = TestSocket('server'),
                 tc = TestSocket('client'),
                 sp = TestSocket._socket_params.copy(),
                 )

    def test_07120_transport_server_defaults(self):
        """07120 masterd transport:   server default attributes"""

        # Test:  transport attributes as expected
        assert_equal(self.ts.service_role, 'server')
        assert_equal(self.ts.service_url, 'tcp://127.0.0.1:9302')
        assert_equal(self.ts.socket_type, zmq.REP)
        assert_equal(self.ts.socket_method, zmq.Socket.bind)

    def test_07121_transport_client_defaults(self):
        """07121 masterd transport:   client default attributes"""

        # Test:  transport attributes as expected
        assert_equal(self.tc.service_role, 'client')
        assert_equal(self.tc.service_url, 'tcp://127.0.0.1:9302')
        assert_equal(self.tc.socket_type, zmq.REQ)
        assert_equal(self.tc.socket_method, zmq.Socket.connect)

    def test_07130_transport_zmq_socket_method(self):
        """07130 masterd transport:  ZMQ socket method"""

        # Setup:  monkey patch socket attributes
        TestSocket._socket_params = self.sp.copy()

        for (method_name, server_method, client_method) in [ \
            ('bind', zmq.Socket.bind, zmq.Socket.connect),
            ('connect', zmq.Socket.connect, zmq.Socket.bind),
            ]:
            # Setup:  set socket method name
            TestSocket._socket_params['method'] = method_name
            
            # Test:  socket's zmq method
            print "Method %s, Server" % TestSocket._socket_params['method']
            assert_equal(self.ts.socket_method, server_method)
            print "Method %s, Client" % TestSocket._socket_params['method']
            assert_equal(self.tc.socket_method, client_method)

        # Reset
            TestSocket._socket_params = self.sp

    def test_07131_transport_zmq_socket_type(self):
        """07131 masterd transport:  ZMQ socket type"""

        # Setup:  monkey patch socket attributes
        TestSocket._socket_params = self.sp.copy()

        for (socket_type, server_type, client_type) in [ \
            ('REQ_REP', zmq.REP, zmq.REQ),
            ('PUB_SUB', zmq.SUB, zmq.PUB),
            ('PAIR', zmq.PAIR, zmq.PAIR),
            ]:
            # Setup:  set socket socket_type name
            TestSocket._socket_params['socket_type'] = socket_type
            
            # Test:  socket's zmq socket_type
            print "Socket type %s, Server:  %s = %s " % \
                (TestSocket._socket_params['socket_type'],
                 TestSocket._type_map[self.ts.socket_type],
                 TestSocket._type_map[server_type])
            assert_equal(self.ts.socket_type, server_type)
            print "Socket type %s, Client:  %s = %s " % \
                (TestSocket._socket_params['socket_type'],
                 TestSocket._type_map[self.tc.socket_type],
                 TestSocket._type_map[client_type])
            assert_equal(self.tc.socket_type, client_type)

        # Reset
            TestSocket._socket_params = self.sp


    def test_07140_transport_zmq_socket_send_recv(self):
        """07140 masterd transport:  ZMQ socket send()/recv()"""

        # Setup: a client socket, and a server socket running in a thread
        # that replies 'pong' after 'ping', or 'not pong: <foo>' after
        # <foo>
        def pingpong_server():
            server_socket = TestSocket('server')
            req = server_socket.recv()
            rep = 'pong' if req == 'ping' else 'not pong: %s' % req
            server_socket.send(rep)
            server_socket.close()
            assert_true(server_socket.closed)
            return
        def pingpong(req):
            thread = threading.Thread(name="pingpong_%s" % req,
                                      target=pingpong_server)
            thread.start()
            client_socket = TestSocket('client')
            client_socket.send(req)
            rep = client_socket.recv()
            client_socket.close()
            assert_true(client_socket.closed)
            thread.join()
            assert_false(thread.is_alive())
            return rep

        # Test: ping->pong
        assert_equal(pingpong('ping'), 'pong')
        assert_equal(pingpong('punk'), 'not pong: punk')


    def test_07141_transport_zmq_socket_recv_timeout(self):
        """07141 masterd transport:  ZMQ socket recv() timeout"""

        # Setup: a server socket with 10ms timeout
        server_socket = TestSocket('server')
        server_socket.receive_timeout_ms = 10

        # Test:  no reply within timeout raises exception
        assert_raises(zmq.error.Again,server_socket.recv)

        # Cleanup
        server_socket.close()
        assert_true(server_socket.closed)

