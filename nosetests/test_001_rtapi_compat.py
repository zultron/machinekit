#!/usr/bin/env python

from . import RTAPITestCase
from nose.tools import assert_equal

from machinekit import compat

class test_001_rtapi_compat(RTAPITestCase):
    def test_00100_compat(self):
        """00100 rtapi compat:  Loading nonexistent module fails"""
        assert_equal(compat.is_module_loaded("foobarbaz"), False)
