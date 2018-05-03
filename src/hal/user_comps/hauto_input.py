#!/usr/bin/env python
#    Copyright 2018 Tormach, Inc
#    Copyright 2007 Jeff Epler <jepler@unpythonic.net>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import linux_event, sys, os, fcntl, hal, select, time, glob, fnmatch, select
from hal import *

# for debugging within Wing IDE
try:
    import wingdbstub
except ImportError:
    pass

def tohalname(s): return str(s).lower().replace("_", "-")

class HalWrapper:
    def __init__(self, comp):
        self._comp = comp
        self._drive = {}
        self._pins = set()
        self._params = set()

    def drive(self):
        for k, v in self._drive.items(): setattr(self._comp, k, v)

    def newpin(self, *args):
        self._pins.add(args[0])
        return self._comp.newpin(*args)
    def newparam(self, *args):
        self._params.add(args[0])
        return self._comp.newparam(*args)

    def __getitem__(self, k):
        if k in self._drive: return self._drive[k]
        return self._comp[k]

    def __setitem__(self, k, v):
        if k in self._params:
            self._comp[k] = v; return
        if k in self._pins:
            self._comp[k] = v; return
        raise KeyError, k
        self._drive[k] = v

class HautoInputDevice:
    def __init__(self, comp, idx, name):
        self.device = linux_event.InputDevice(name)

        self.idx = idx
        self.codes = {}
        self.last = {}
        self.rel_items = []
        self.abs_items = []
        self.comp = comp

        self.codes['BTN_0'] = "button.feedhold"
        comp.newpin("%s.%s" % (idx, self.codes['BTN_0']), HAL_BIT, HAL_OUT)
        comp.newpin("%s.%s-not" % (idx, self.codes['BTN_0']), HAL_BIT, HAL_OUT)
        self.set(self.codes['BTN_0'] + "-not", 1)

        self.codes['BTN_1'] = "button.cycle-start"
        comp.newpin("%s.%s" % (idx, self.codes['BTN_1']), HAL_BIT, HAL_OUT)
        comp.newpin("%s.%s-not" % (idx, self.codes['BTN_1']), HAL_BIT, HAL_OUT)
        self.set(self.codes['BTN_1'] + "-not", 1)

        self.ledmap = {
             "led.ready": 'LED_MISC',
            "led.board": 'LED_KANA',
        }
        for name, ledcode in self.ledmap.items():
            comp.newpin("%s.%s" % (idx, name), HAL_BIT, HAL_IN)
            comp.newpin("%s.%s-invert" % (idx, name), HAL_BIT, HAL_IN)
            self.last[name] = 0
            self.device.write_event('EV_LED', ledcode, 0)

        self.codes['ABS_RX'] = 'axis-select'
        absinfo = self.device.get_absinfo('ABS_RX')
        comp.newpin("%s.%s-counts" % (idx, self.codes['ABS_RX']), HAL_S32, HAL_OUT)
        self.set(self.codes['ABS_RX'] + "-counts", absinfo.value)
        for i in range(4):
            comp.newpin("%s.%s" % (idx, "axis.{}.enabled".format(i)), HAL_BIT, HAL_OUT)

        self.codes['ABS_RY'] = 'step-select'
        absinfo = self.device.get_absinfo('ABS_RY')
        comp.newpin("%s.%s-counts" % (idx, self.codes['ABS_RY']), HAL_S32, HAL_OUT)
        comp.newpin("%s.%s-value" % (idx, self.codes['ABS_RY']), HAL_FLOAT, HAL_OUT)
        comp.newpin("%s.%s-scale" % (idx, self.codes['ABS_RY']), HAL_FLOAT, HAL_IN)
        self.set(self.codes['ABS_RY'] + "-counts", absinfo.value)
        self.set(self.codes['ABS_RY'] + "-scale", 0.0001)

        self.codes['ABS_WHEEL'] = 'jog-wheel'
        absinfo = self.device.get_absinfo('ABS_WHEEL')
        comp.newpin("%s.%s-counts" % (idx, self.codes['ABS_WHEEL']), HAL_S32, HAL_OUT)
        comp.newpin("%s.%s-count-div" % (idx, self.codes['ABS_WHEEL']), HAL_S32, HAL_IN)
        comp.newpin("%s.%s-counts-adjusted" % (idx, self.codes['ABS_WHEEL']), HAL_S32, HAL_OUT)
        self.set(self.codes['ABS_WHEEL'] + "-count-div", 4)
        self.set(self.codes['ABS_WHEEL'] + "-counts", absinfo.value)

        self.codes['ABS_X'] = 'feed-override'
        self.codes['ABS_Y'] = 'rpm-override'
        self.codes['ABS_Z'] = 'rapid-override'


        for name in [self.codes['ABS_X'], self.codes['ABS_Y'], self.codes['ABS_Z']]:
            comp.newpin("%s.%s-position" % (idx, name), HAL_FLOAT, HAL_OUT)
            comp.newpin("%s.%s-counts" % (idx, name), HAL_S32, HAL_OUT)
            comp.newpin("%s.%s-scale" % (idx, name), HAL_FLOAT, HAL_IN)
            comp.newpin("%s.%s-center" % (idx, name), HAL_FLOAT, HAL_IN)
            comp.newpin("%s.%s-flat" % (idx, name), HAL_FLOAT, HAL_IN)
            self.set(name + "-counts", 0)
            self.set(name + "-position", 0)
            self.set(name + "-scale", 682)
            # "flat" is the deadzone around "center" where the value is set to center
            self.set(name + "-center", 1.0)
            self.set(name + "-flat", 0.05)
            self.abs_items.append(name)

        self.update()

        
    def get(self, name):
        name = "%s.%s" % (self.idx, name)
        return self.comp[name]

    def set(self, name, value):
        name = "%s.%s" % (self.idx, name)
        self.comp[name] = value

    def update(self):
        while self.device.readable():
            ev = self.device.read_event()
            if ev.type == 'EV_SYN': continue
            elif ev.type == 'EV_SND': continue
            elif ev.type == 'EV_MSC': continue
            elif ev.type == 'EV_LED': continue

            code = ev.code
            if code not in self.codes:
                print >>sys.stderr, "Unexpected event", ev.type, ev.code
                continue
            if ev.type == 'EV_KEY':
                if ev.value:
                    self.set(self.codes[code], 1)
                    self.set(self.codes[code] + "-not", 0)
                else:
                    self.set(self.codes[code], 0)
                    self.set(self.codes[code] + "-not", 1)
            elif ev.type == 'EV_ABS':
                value = ev.value
                self.set(self.codes[code] + "-counts", value)

        # Jog wheel
        halname = self.codes['ABS_WHEEL']
        value = self.get(halname + "-counts")
        countDiv = self.get(halname + "-count-div")
        value = int(value) / int(countDiv)
        self.set(halname + "-counts-adjusted", value)

        # axis select
        halname = self.codes['ABS_RX']
        value = self.get(halname + "-counts") - 1
        # counts starts from 0 for OFF, so shift it so -1 is OFF and 0 is X etc
        for i in range(4):
            self.set("axis.{}.enabled".format(i), 0)
        # Make sure this is a valid axis for the hauto to enable, otherwise don't enable one at all
        if value in range(4):
            self.set("axis.{}.enabled".format(value), 1)

        # step select
        halname = self.codes['ABS_RY']
        value = self.get(halname + "-counts")
        scale = self.get(halname + "-scale")
        self.set(halname + "-value", scale * 10**value)

        # override knobs
        for a in self.abs_items:
            value = self.get(a + "-counts")
            scale = self.get(a + "-scale") or 1
            center = self.get(a + "-center")
            flat = self.get(a + "-flat")
            position = (value / scale)
            if (center-flat) <= position <= (center+flat):
                position=center
            self.set(a + "-position", position)

        for halname, v in self.last.items():
            # Note: this is OK because the hal module always returns True or False for HAL_BIT values
            u = self.comp["%s.%s" % (self.idx, halname)] != self.comp["%s.%s-invert" % (self.idx, halname)]
            if u != v:
                led = self.ledmap[halname]
                self.device.write_event('EV_LED', led, u)
                self.last[halname] = u

h = component("hauto_input")
w = HalWrapper(h)
h.setprefix("hauto")
d = []
i = 0
try:
    d.append(HautoInputDevice(w, 0, "FranksWorkshop"))
except LookupError, detail:
    raise SystemExit, detail

w.drive()
h.ready()

fds = [dev.device.fileno() for dev in d]
try:
    while 1:
        select.select(fds, [], [], .01)
        for i in d: i.update()
        w.drive()
except KeyboardInterrupt:
    pass
