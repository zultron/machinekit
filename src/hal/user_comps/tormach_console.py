#!/usr/bin/env python
#    Copyright 2018 Tormach, Inc
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

from hal import *
import evdev
from evdev import ecodes
from select import select
from itertools import count, ifilterfalse

# for debugging within Wing IDE
try:
    import wingdbstub
except ImportError:
    pass

NUMBER_SUPPORTED = 1

def tohalname(s): return str(s).lower().replace("_", "-")

class HalWrapper:
    def __init__(self, comp):
        self._comp = comp
        self._pins = set()
        self._params = set()

    def newpin(self, *args):
        self._pins.add(args[0])
        return self._comp.newpin(*args)
    def newparam(self, *args):
        self._params.add(args[0])
        return self._comp.newparam(*args)

    def __getitem__(self, k):
        return self._comp[k]

    def __setitem__(self, k, v):
        if k in self._params:
            self._comp[k] = v; return
        elif k in self._pins:
            self._comp[k] = v; return
        else: 
            raise KeyError, k


class TormachConsoleInputDevice:
    def __init__(self, wrapped_comp, index, event_device):
        self.event_device = event_device
        self.wrapped_comp = wrapped_comp
        self.index = index
        # self.codes os the mapping of HID device inputs to HAL pins
        self.codes = {
            'BTN_0': "button.feedhold",
            'BTN_1':"button.cycle-start",
            'ABS_RX':"axis-select",
            'ABS_RY':'step-select',
            'ABS_X':'feed-override',
            'ABS_Y':'rpm-override',
            'ABS_Z':'rapid-override',
            'ABS_WHEEL':'jog-wheel',
            'LED_NUML': "led.ready",
            'LED_CAPSL': "led.board",
            'LED_SCROLLL': "led.red",
            'LED_COMPOSE': "led.green",
            'LED_KANA': "led.blue"
        }
        self.buttons = ['BTN_0', 'BTN_1']
        self.leds = ['LED_NUML', 'LED_CAPSL', 'LED_SCROLLL', 'LED_COMPOSE', 'LED_KANA']

        for button in self.buttons:
            self.newpin(self.codes[button], HAL_BIT, HAL_OUT)
            self.newpin("%s-latch" % self.codes[button], HAL_BIT, HAL_IO)
            self.set(self.codes[button], 0)
            self.set(self.codes[button] + "-latch", 0)

        for led in self.leds:
            self.newpin(self.codes[led], HAL_BIT, HAL_IN)
            self.newpin("%s-invert" % self.codes[led], HAL_BIT, HAL_IN)
            self.set(self.codes[led], 0)
            self.set(self.codes[led] + "-invert", 0)

        self.newpin("%s-counts" % self.codes['ABS_RX'], HAL_S32, HAL_OUT)
        for i in range(4):
            self.newpin("%s" % "axis.{}.enabled".format(i), HAL_BIT, HAL_OUT)

        self.newpin("%s-counts" % self.codes['ABS_RY'], HAL_S32, HAL_OUT)
        self.newpin("%s-value" % self.codes['ABS_RY'], HAL_FLOAT, HAL_OUT)
        self.newpin("%s-scale" % self.codes['ABS_RY'], HAL_FLOAT, HAL_IN)
        self.set(self.codes['ABS_RY'] + "-scale", 0.0001)

        self.newpin("%s-counts" % self.codes['ABS_WHEEL'], HAL_S32, HAL_OUT)
        self.newpin("%s-count-div" % self.codes['ABS_WHEEL'], HAL_S32, HAL_IN)
        self.newpin("%s-counts-adjusted" % self.codes['ABS_WHEEL'], HAL_S32, HAL_OUT)
        self.set(self.codes['ABS_WHEEL'] + "-count-div", 4)

        for name in [self.codes['ABS_X'], self.codes['ABS_Y'], self.codes['ABS_Z']]:
            self.newpin("%s-position" % name, HAL_FLOAT, HAL_OUT)
            self.newpin("%s-counts" % name, HAL_S32, HAL_OUT)
            self.newpin("%s-scale" % name, HAL_FLOAT, HAL_IN)
            self.newpin("%s-center" % name, HAL_FLOAT, HAL_IN)
            self.newpin("%s-flat" % name, HAL_FLOAT, HAL_IN)
            self.set(name + "-counts", 0)
            self.set(name + "-position", 0)
            self.set(name + "-scale", 512)
            # "flat" is the deadzone around "center" where the svalue is set to center
            self.set(name + "-center", 1.0)
            self.set(name + "-flat", 0.05)

    def update(self):
        for event in self.event_device.read():
            event = evdev.categorize(event)
            if isinstance(event, evdev.KeyEvent):
                code = event.keycode
                if not isinstance(code, basestring):
                    code = code[0] #sometimes multiple keycodes are returned for one button
                if code in self.codes:
                    self.set(self.codes[code], event.keystate != 0)
                    if(event.keystate == event.key_up): #only set the latch pin on a key release event
                        self.set(self.codes[code] + "-latch", 1)

            if isinstance(event, evdev.AbsEvent) and ecodes.ABS[event.event.code] in self.codes:
                self.set(self.codes[ecodes.ABS[event.event.code]] + "-counts", event.event.value)

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
            for code in ['ABS_X', 'ABS_Y', 'ABS_Z']:
                name = self.codes[code]
                value = self.get(name + "-counts")
                scale = self.get(name + "-scale") or 1
                center = self.get(name + "-center")
                flat = self.get(name + "-flat")
                position = round((value / scale), 2)
                if (center-flat) <= position <= (center+flat):
                    position=center
                self.set(name + "-position", position)

            current_active_leds = [l[0] for l in self.event_device.leds(verbose=True)]
            #write led values
            for led in self.leds:
                value = led in current_active_leds 
                #if this name is in the active list, then the value is True
                halname = self.codes[led]
                hal_val = self.wrapped_comp["%s.%s" % (self.index, halname)] != self.wrapped_comp["%s.%s-invert" % (self.index, halname)]
                if hal_val != value:
                    self.event_device.set_led(ecodes.ecodes[led], not value)


    def get(self, name):
        name = "%s.%s" % (self.index, name)
        return self.wrapped_comp[name]

    def set(self, name, value):
        name = "%s.%s" % (self.index, name)
        self.wrapped_comp[name] = value

    def newpin(self, name, *args):
        name = "%s.%s" % (self.index, name)
        if name not in self.wrapped_comp._pins:
            self.wrapped_comp.newpin(name, *args)

    def device_connected(self):
        return self.event_device != None

import time, os

hal_component = component("tormach_console")
wrapper = HalWrapper(hal_component)
hal_component.setprefix("tormach-console")

#We need to initialize these devices up front because HAL needs all pins created now
devices = [TormachConsoleInputDevice(wrapper, index, None) for index in range(NUMBER_SUPPORTED)]

def connected_devices():
    return [dev for dev in devices if dev.device_connected()]

#If we have new USB devices, we find the first TormachConsoleInputDevice where event_device==None
#and set the event_device to the new device
def check_for_devices():
    device_list = [dev for dev in [evdev.InputDevice(path) for path in evdev.list_devices()] if dev.name=="FranksWorkshop Generic HID Device"]
    for dev in device_list:
        if dev not in [idev.event_device for idev in devices]:
            indexes = [idev.index for idev in devices if idev.device_connected()]
            try:
                next_index = next(ifilterfalse(set(indexes).__contains__, range(NUMBER_SUPPORTED)))
            except StopIteration as e:
                print "More console devices were plugged in than are supported on this system."
            devices[next_index].event_device = dev
    #print len(connected_devices()), "console devices found"

check_for_devices()
wrapper.newpin('device-connected', HAL_BIT, HAL_OUT)

hal_component.ready()

last_check=time.time()

while True:
    # Check for new console devices every 5 seconds
    if time.time() - last_check > 5:
        check_for_devices()
        last_check = time.time()
    if len( connected_devices() ) == 0:
        wrapper['device-connected']=0
        time.sleep(1)
    else:
        wrapper['device-connected']=1
    try:
        fds = {dev.event_device.fd: dev for dev in devices if dev.device_connected()}
        readable, w, x = select(fds.keys(), [], [], 1.0)
        for fd in readable:
            fds[fd].update()
    except IOError as e:
        for dev in connected_devices():
            if os.fstat(dev.event_device.fd).st_mode == 8192:
                print "Tormach console device", dev.index, "dissapeared"
                dev.event_device = None