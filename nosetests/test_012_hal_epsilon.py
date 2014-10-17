from utils import RTAPITestCase
from nose.tools import assert_equal, assert_almost_equal, assert_in, \
    assert_greater, assert_false, assert_true, \
    assert_is_none, assert_is_not_none, assert_is_instance, \
    assert_raises

from machinekit import hal

class test_012_hal_epsilon(RTAPITestCase):

    def test_01210_init_component(self):
        """01210 hal epsilon:   Initialize component"""
        # custom deltas (leave epsilon[0] - the default - untouched)
        hal.epsilon[1] = 100.0
        hal.epsilon[2] = 1000.0

        self.f.epstestcomp = hal.Component('epstest')

    def test_01220_ccomp_and_epsilon(self):
        """01220 hal epsilon:   Test pin change detection"""
        # select epsilon[1] for change detection on 'out1'
        # means: out1 must change by more than 100.0 to report a changed pin
        p1 = self.f.epstestcomp.newpin("out1", hal.HAL_FLOAT, hal.HAL_OUT, eps=1)

        # but use epsilon[2] - 1000.0 for out2
        p2 = self.f.epstestcomp.newpin("out2", hal.HAL_FLOAT, hal.HAL_OUT, eps=2)
        self.f.epstestcomp.ready()
        print p1.eps, p1.epsilon, p2.eps, p2.epsilon

        # we havent changed pins yet from default, so
        # self.f.epstestcomp.changed() reports 0 (the number of pin
        # changes detected)
        assert_equal(self.f.epstestcomp.changed(), 0)

        pinlist = []

        # report_all=True forces a report of all pins
        # regardless of change status, so 2 pins to report:
        self.f.epstestcomp.changed(userdata=pinlist,report_all=True)
        assert_equal(len(pinlist), 2)

        # passing a list as 'userdata=<list>' always clears the list before
        # appending pins
        self.f.epstestcomp.changed(userdata=pinlist,report_all=True)
        assert_equal(len(pinlist), 2)

        self.f.epstestcomp["out1"] += 101  # larger than out1's epsilon value
        self.f.epstestcomp["out2"] += 101  # smaller than out2's epsilon value

        # this must result in out1 reported changed, but not out2
        self.f.epstestcomp.changed(userdata=pinlist)
        assert_equal(len(pinlist), 1)

        self.f.epstestcomp["out2"] += 900  # exceed out2's epsilon value
                                         # in second update:
        self.f.epstestcomp.changed(userdata=pinlist)
        assert_equal(len(pinlist), 1)

        # since no changes since last report, must be 0:
        assert_equal(self.f.epstestcomp.changed(), 0)

    def test_01290_teardown_class(self):
        """01290 hal epsilon:   Stop component"""
        self.f.epstestcomp.exit()
