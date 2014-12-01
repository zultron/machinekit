from . import FixtureTestCase
from unittest import skip, SkipTest
from nose.tools import assert_equal, assert_not_equal, assert_almost_equal, \
    assert_in, assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

import os, zmq

class test_070_masterd_data_encapsulation(FixtureTestCase):

    def test_07010_init_config(self):
        """07010 masterd data encapsulation:  init config"""

        # Build the python protobuf test service module
        os.system("protoc \
	    --python_out=%(filesdir)s \
	    --proto_path=%(filesdir)s \
	    %(filesdir)s/test_07x_masterd_service.proto" % \
                      dict(filesdir = self.filesdir))

    def test_07020_load_objects(self):
        """07020 masterd data encapsulation:  load test service objects"""

        from files.test_07x_masterd_service import *
        self.fix(test_service = TestService)

    def test_07030_protobuf_fields(self):
        """07030 masterd data encapsulation:  protobuf fields"""

        # Setup:  test message
        client = self.test_service.client()
        request1 = client.request(
            type = 10,  # TEST1
            name = "test_request",
            strvalue = "a string",
            intvalue = 42,
            boolvalue = True,
            )

        # Test:  message fields come out as they went in
        assert_equal(request1.type, 10)
        assert_equal(request1.name, "test_request")
        assert_equal(request1.strvalue, "a string")
        assert_equal(request1.intvalue, 42)
        assert_true(request1.boolvalue)

        # Save message for next test
        self.fix(request1 = request1)

    def test_07040_protobuf_encap(self):
        """07040 masterd data encapsulation:  protobuf encapsulation"""

        # Setup:  serialize and deserialize
        request1 = self.request1
        request2 = self.test_service.request_type.deserialize(
            request1.serialize())

        # Test:  message fields match
        assert_equal(request1.type, request2.type)
        assert_equal(request1.name, request2.name)
        assert_equal(request1.strvalue, request2.strvalue)
        assert_equal(request1.intvalue, request2.intvalue)
        assert_equal(request1.boolvalue, request2.boolvalue)

    def test_07050_method_name(self):
        """07050 masterd data encapsulation:  method_name property"""

        # Test:  default method_name property returns expected value
        assert_equal(self.request1.method_name, 'request_test1')

    def test_07060_server_reply(self):
        """07060 masterd data encapsulation:  server reply"""

        # Setup: get server instance; use previous request1 to
        # generate reply
        server = self.test_service.server()
        reply1 = server.reply_type.build_from_request(self.request1)
        self.log.info("Received reply <%s>", reply1)

        # Test:  reply fields returned as expected
        assert_equal(reply1.type, 10)
        assert_equal(reply1.name, "test_reply")
        assert_equal(reply1.strvalue, "a string reply")
        assert_equal(reply1.intvalue, 13)
        assert_false(reply1.boolvalue)

