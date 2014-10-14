#!/usr/bin/env python

from utils import RTAPITestCase
from proboscis import test
from nose.tools import assert_equal

from machinekit import compat

@test(groups=["rtapi"])
class TestCompat(RTAPITestCase):
    @test
    def compat(self):
        """Compat:  Loading nonexistent module fails"""
        assert_equal(compat.is_module_loaded("foobarbaz"), False)
