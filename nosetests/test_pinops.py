#!/usr/bin/env python

from utils import RTAPITestCase, check_hal_clean
from proboscis import test, before_class, after_class
from nose.tools import assert_raises, assert_equal, assert_almost_equal, \
    assert_true, assert_false

from machinekit import hal

@test(groups=["hal","hal_pins"],
      depends_on_groups=["hal_base"])
class TestPinOps(RTAPITestCase):
    places = 6
    pins = [
        ("s32out", hal.HAL_S32, hal.HAL_OUT,42),
        ("s32in", hal.HAL_S32, hal.HAL_IN, 42),
        ("s32io", hal.HAL_S32, hal.HAL_IO, 42),

        ("u32out", hal.HAL_U32, hal.HAL_OUT, 123),
        ("u32in", hal.HAL_U32, hal.HAL_IN, 123),
        ("u32io", hal.HAL_U32, hal.HAL_IO, 123),

        ("floatout", hal.HAL_FLOAT, hal.HAL_OUT, 3.14),
        ("floatin", hal.HAL_FLOAT, hal.HAL_IN, 3.14),
        ("floatio", hal.HAL_FLOAT, hal.HAL_IO, 3.14),

        ("bitout", hal.HAL_BIT, hal.HAL_OUT, True),
        ("bitin", hal.HAL_BIT, hal.HAL_IN, True),
        ("bitio", hal.HAL_BIT, hal.HAL_IO, True),
        ]

    @before_class
    def init_pins(self):
        """Pinops:  Initialize pins"""
        c1 = hal.Component("c1")

        for p in self.pins:
            setattr(self, p[0], c1.newpin(*p[0:3], init=p[3]))

        c1.ready()

    @test
    def getters(self):
        """Pinops:  Check getters """

        assert_equal(self.s32out.get(), 42)
        assert_equal(self.s32in.get(), 42)
        assert_equal(self.s32io.get(), 42)

        assert_equal(self.u32out.get(), 123)
        assert_equal(self.u32in.get(), 123)
        assert_equal(self.u32io.get(), 123)

        assert_true(self.bitout.get())
        assert_true(self.bitin.get())
        assert_true(self.bitio.get())

        assert_almost_equal(self.floatout.get(),
                            3.14, places=self.places)
        assert_almost_equal(self.floatin.get(),
                            3.14, places=self.places)
        assert_almost_equal(self.floatio.get(),
                            3.14, places=self.places)

    @test(depends_on=[getters])
    def setters(self):
        """Pinops:  Check setters """

        assert_equal(self.s32out.set(4711), 4711)
        assert_equal(self.s32in.set(4711), 4711)
        assert_equal(self.s32io.set(4711), 4711)
        assert_raises(RuntimeError, self.s32out.set, 39.5)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError, self.s32out.set, True)

        assert_equal(self.u32out.set(815), 815)
        assert_equal(self.u32in.set(815), 815)
        assert_equal(self.u32io.set(815), 815)
        assert_raises(RuntimeError,self.u32in.set, 39.5)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError,self.u32io.set, -815)
        #assert_raises(RuntimeError,self.u32in.set, False)

        assert_false(self.bitout.set(False))
        assert_false(self.bitin.set(False))
        assert_false(self.bitio.set(False))
        assert_raises(RuntimeError,self.bitout.set, 39.5)

        assert_almost_equal(self.floatout.set(2.71828),2.71828,
                            places=self.places)
        assert_almost_equal(self.floatin.set(2.71828) ,2.71828,
                            places=self.places)
        assert_almost_equal(self.floatio.set(2.71828) ,2.71828,
                            places=self.places)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError, self.floatio.set, True)

    @after_class
    def stop_component(self):
        """Pinops:  Stop component"""
        hal.Component('c1',wrap=True).exit()

        check_hal_clean()
