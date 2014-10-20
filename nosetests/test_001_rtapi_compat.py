#!/usr/bin/env python

from . import RTAPITestCase
from nose.tools import assert_equal, assert_raises, assert_true

from machinekit import compat

class test_001_rtapi_compat(RTAPITestCase):
    flavorlist = ("posix", "rt-preempt", "xenomai",
                   "xenomai-kernel", "rtai-kernel")

    def test_00100_compat(self):
        """00100 rtapi compat:  Loading nonexistent module fails"""
        assert_equal(compat.is_module_loaded("foobarbaz"), False)

    def test_00110_kernel_type(self):
        """00110 rtapi compat:  Detect kernel type"""
        # Kernel should be only one type (this may not always be true)
        true_count = 0
        self.f.maybe_have_kthreads = False

        self.f.kernel_rtpreempt = compat.kernel_is_rtpreempt()
        if self.f.kernel_rtpreempt:
            true_count += 1
            self.f.default_flavor = "rt-preempt"

        self.f.kernel_xenomai = compat.kernel_is_xenomai()
        if self.f.kernel_xenomai:
            true_count += 1
            self.f.default_flavor = "xenomai"
            self.f.maybe_have_kthreads = True

        self.f.kernel_rtai = compat.kernel_is_rtai()
        if self.f.kernel_rtai:
            true_count += 1
            self.f.default_flavor = "rtai-kernel"
            self.f.maybe_have_kthreads = True

        if true_count == 0:
            self.f.kernel_vanilla = True
            true_count = 1
            self.f.default_flavor = "posix"
        else:
            self.f.kernel_vanilla = False

        assert_equal(true_count,1)

    def test_00120_default_flavor(self):
        """00120 rtapi compat:  Detect default flavor"""
        assert_equal(self.f.default_flavor, compat.default_flavor().name)

    def test_00121_flavor_byname(self):
        """00121 rtapi compat:  Query flavor by name"""
        for f in self.flavorlist:
            assert_equal(compat.flavor_byname(f).name,f)

    def test_00122_flavor_byid(self):
        """00122 rtapi compat:  Query flavor by ID"""
        # reverse-lookup: check that id matches that of the flavor
        # looked up by name
        for i in range(len(self.flavorlist)):
            f = compat.flavor_byid(i)
            frev = compat.flavor_byname(f.name)
            assert_equal(i,frev.id)

    def test_00130_module_path(self):
        """00130 rtapi compat:  Module path"""
        if not self.f.maybe_have_kthreads:
            # Userland threads will throw an exception
            assert_raises(RuntimeError,compat.module_path,"abs")
        else:
            if self.f.kernel_rtai:
                assert_true(compat.module_path("abs").endswith("abs.ko"))
            else:
                # Having a Xenomai kernel doesn't necessarily mean having
                # built xenomai-kernel threads
                try:
                    compat.module_path("abs")
                    self.f.kernel_xenomai_kthreads = True
                except RuntimeError:
                    self.f.kernel_xenomai_kthreads = False
                if self.f.kernel_xenomai_kthreads:
                    assert_true(compat.module_path("abs").endswith("abs.ko"))


    def test_00140_rtapi_config_global(self):
        """00140 rtapi compat:  Read rtapi.ini global params"""
        print "DEBUG = %d" % int(compat.get_rtapi_config("DEBUG"))
        assert_true(int(compat.get_rtapi_config("DEBUG")) in (1,2,3,4,5))
        print "USE_SHMDRV = %s" % compat.get_rtapi_config("USE_SHMDRV")
        assert_true(compat.get_rtapi_config("USE_SHMDRV") in ("yes","no"))
        print "flavor = %s" % compat.get_rtapi_config("flavor")
        assert_true(compat.get_rtapi_config("flavor").endswith("/flavor"))
        print "rtapi_msgd = %s" % compat.get_rtapi_config("rtapi_msgd")
        assert_true(compat.get_rtapi_config(
                "rtapi_msgd").endswith("/rtapi_msgd"))
        print "linuxcnc_module_helper = %s" % \
            compat.get_rtapi_config("linuxcnc_module_helper")
        assert_true(compat.get_rtapi_config(
                "linuxcnc_module_helper").endswith("/linuxcnc_module_helper"))
        
    def test_00141_rtapi_config_flavor(self):
        """00141 rtapi compat:  Read rtapi.ini flavor params"""
        assert_true(compat.get_rtapi_config(
                "rtapi_app").endswith("/rtapi_app_%s" % \
                                          self.f.default_flavor))
