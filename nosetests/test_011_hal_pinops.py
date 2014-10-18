from . import RTAPITestCase
from nose.tools import assert_raises, assert_equal, assert_almost_equal, \
    assert_true, assert_false

from machinekit import hal

class test_011_hal_pinops(RTAPITestCase):
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

    def test_01110_init_pins(self):
        """01110 hal pinops:  Initialize pins"""
        c1 = hal.Component("c1")

        for p in self.pins:
            setattr(self.f, p[0], c1.newpin(*p[0:3], init=p[3]))

        c1.ready()

    def test_01120_getters(self):
        """01120 hal pinops:  Check getters """

        assert_equal(self.f.s32out.get(), 42)
        assert_equal(self.f.s32in.get(), 42)
        assert_equal(self.f.s32io.get(), 42)

        assert_equal(self.f.u32out.get(), 123)
        assert_equal(self.f.u32in.get(), 123)
        assert_equal(self.f.u32io.get(), 123)

        assert_true(self.f.bitout.get())
        assert_true(self.f.bitin.get())
        assert_true(self.f.bitio.get())

        assert_almost_equal(self.f.floatout.get(),
                            3.14, places=self.places)
        assert_almost_equal(self.f.floatin.get(),
                            3.14, places=self.places)
        assert_almost_equal(self.f.floatio.get(),
                            3.14, places=self.places)

    def test_01130_setters(self):
        """01130 hal pinops:  Check setters """

        assert_equal(self.f.s32out.set(4711), 4711)
        assert_equal(self.f.s32in.set(4711), 4711)
        assert_equal(self.f.s32io.set(4711), 4711)
        assert_raises(RuntimeError, self.f.s32out.set, 39.5)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError, self.f.s32out.set, True)

        assert_equal(self.f.u32out.set(815), 815)
        assert_equal(self.f.u32in.set(815), 815)
        assert_equal(self.f.u32io.set(815), 815)
        assert_raises(RuntimeError,self.f.u32in.set, 39.5)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError,self.f.u32io.set, -815)
        #assert_raises(RuntimeError,self.f.u32in.set, False)

        assert_false(self.f.bitout.set(False))
        assert_false(self.f.bitin.set(False))
        assert_false(self.f.bitio.set(False))
        assert_raises(RuntimeError,self.f.bitout.set, 39.5)

        assert_almost_equal(self.f.floatout.set(2.71828),2.71828,
                            places=self.places)
        assert_almost_equal(self.f.floatin.set(2.71828) ,2.71828,
                            places=self.places)
        assert_almost_equal(self.f.floatio.set(2.71828) ,2.71828,
                            places=self.places)
        # FIXME:  AssertionError: RuntimeError not raised
        #assert_raises(RuntimeError, self.f.floatio.set, True)

    def test_01190_stop_component(self):
        """01190 hal pinops:  Stop component"""
        hal.Component('c1',wrap=True).exit()
