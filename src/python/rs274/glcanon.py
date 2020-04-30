#    This is a component of AXIS, a front-end for emc
#    Copyright 2004, 2005, 2006 Jeff Epler <jepler@unpythonic.net>
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

from rs274 import Translated, ArcsToSegmentsMixin, OpenGLTk
from minigl import *
import math
import glnav
from gl_context_managers import *
import hershey
import linuxcnc
import array
import gcode
import os
from collections import defaultdict, namedtuple
import time
import struct
import operator
import numpy as np
import re


MotionKey = namedtuple('MotionKey', ['motion_type', 'exec_mode', 'tool_no'])


def minmax(*args):
    return min(*args), max(*args)

homeicon = array.array('B',
        [0x2, 0x00,   0x02, 0x00,   0x02, 0x00,   0x0f, 0x80,
        0x1e, 0x40,   0x3e, 0x20,   0x3e, 0x20,   0x3e, 0x20,
        0xff, 0xf8,   0x23, 0xe0,   0x23, 0xe0,   0x23, 0xe0,
        0x13, 0xc0,   0x0f, 0x80,   0x02, 0x00,   0x02, 0x00])

limiticon = array.array('B',
        [  0,   0,  128, 0,  134, 0,  140, 0,  152, 0,  176, 0,  255, 255,
         255, 255,  176, 0,  152, 0,  140, 0,  134, 0,  128, 0,    0,   0,
           0,   0,    0, 0])


class GLCanon(Translated, ArcsToSegmentsMixin):
    lineno = -1

    def __init__(self, color_config, geometry, inifile):
        """

        :type color_config: GlDrawColors
        :type geometry: str
        :param inifile:
        """
        ArcsToSegmentsMixin.__init__(self)

        self.color_config = color_config
        # Fields that store preview data built up during parse:

        # Dict with tuple key holding various sets of motions
        # {(motion_type_str, exec_mode, tool_number) : (line number, [start position], [end position], feedrate, [tlo x, tlo y, tlo z], toolno)}
        self.tool_motions = defaultdict(list)
        self.tool_motion_types = set()

        self.exec_mode = gcode.PARSE_EXEC_SEEK
        # dwell list - (line number, color, pos x, pos y, pos z, plane)
        self.dwells = []
        # spindle-synched feed list - (line number, (position deltas), S, fpr)
        self.feed_synched = []
        self.feedrate = 1
        self.lo = (0,) * 9
        self.first_move = True
        self.in_arc = 0
        self.xo = self.yo = self.zo = self.ao = self.bo = self.co = self.uo = self.vo = self.wo = 0
        self.dwell_time = 0
        self.suppress = 0
        Translated.__init__(self, 1)

        # Computed once after parse completes then cached here
        self.min_extents = [9e99, 9e99, 9e99]
        self.max_extents = [-9e99, -9e99, -9e99]
        self.min_extents_notool = [9e99, 9e99, 9e99]
        self.max_extents_notool = [-9e99, -9e99, -9e99]

        # Settings at init time that control preview behavior
        self.rear_tp_lathe_option = bool(geometry == "-XZ")
        self.foam_option = bool(inifile.find("DISPLAY", "FOAM"))
        self.foam_z = 0.0
        self.foam_w = 1.5
        self.notify = 0
        self.notify_message = ""
        self.visible_tools = None
        self.current_tool = 0

        # KLUDGE Define these attributes here for mixins only
        self.choice = None
        self.tool_data_capsule = None

    def comment(self, arg):
        if arg.startswith("AXIS,"):
            parts = arg.split(",")
            command = parts[1]
            if command == "stop": raise KeyboardInterrupt
            if command == "hide": self.suppress += 1
            if command == "show": self.suppress -= 1
            if command == "notify":
                self.notify = self.notify + 1
                self.notify_message = "(AXIS,notify):" + str(self.notify)
                if len(parts) > 2:
                    if len(parts[2]): self.notify_message = parts[2]

    def message(self, message): pass

    def check_abort(self): pass

    # For managing motion keys
    def set_exec_mode(self, exec_mode):
        self.exec_mode = exec_mode

    def next_line(self, st):
        pass

    def colored_lines_by_type(self, color_name, motion_key, for_selection):
        if not for_selection:
            self.color_config.gl_color_with_alpha(color_name)
        return linuxcnc.draw_lines_by_type(self.tool_data_capsule, motion_key, for_selection)

    def draw_dwells(self, dwells, alpha, for_selection):
        return linuxcnc.draw_dwells(dwells, alpha, for_selection, self.is_lathe())

    def calc_extents(self):
        self.min_extents, self.max_extents, self.min_extents_notool, self.max_extents_notool = gcode.calc_extents_dict(self.tool_motions)
        if self.foam_option:
            min_z = min(self.foam_z, self.foam_w)
            max_z = max(self.foam_z, self.foam_w)
            self.min_extents = self.min_extents[0], self.min_extents[1], min_z
            self.max_extents = self.max_extents[0], self.max_extents[1], max_z
            self.min_extents_notool = \
                self.min_extents_notool[0], self.min_extents_notool[1], min_z
            self.max_extents_notool = \
                self.max_extents_notool[0], self.max_extents_notool[1], max_z

    def set_extents(self, min_extents, max_extents, min_extents_notool, max_extents_notool):
        # TODO support foam option
        self.min_extents = min_extents
        self.max_extents = max_extents
        self.min_extents_notool = min_extents_notool
        self.max_extents_notool = max_extents_notool

    def calc_velocity(self, delta, axis_max_vel):
        """Using techniques from getStraightVelocity() in emccanon.cc, given 9
        axis deltas and velocity limits, calculate max velocity of a
        straight move; deltas should be absolute; invalid axes should be 0
        """
        # Clean up tiny values
        delta = tuple([(0.0 if i<1e-7 else i) for i in delta])
        # Fastest time of coordinated move is the maximum time of any
        # one axis to perform move at axis max velocity
        tmax = max([(i[0]/i[1] if i[1] else 0.0)
                     for i in zip(delta, axis_max_vel)])
        # Total distance is the hypotenuse of a set of three axes;
        # which set depends on the type of move
        if sum(delta[0:3]) > 0:
            # Linear XYZ with or without ABC or UVW
            dtot = math.sqrt(sum(i*i for i in delta[0:3]))
        elif sum(delta[6:9]) > 0:
            # Linear UVW without XYZ and with or without ABC
            dtot = math.sqrt(sum(i*i for i in delta[6:9]))
        else:
            # Angular-only
            dtot = math.sqrt(sum(i*i for i in delta[3:6]))
        # Max velocity = total distance / fastest time
        max_vel = dtot/tmax
        return max_vel

    def tool_offset(self, xo, yo, zo, ao, bo, co, uo, vo, wo):
        self.first_move = True
        x, y, z, a, b, c, u, v, w = self.lo
        self.lo = (x - xo + self.xo, y - yo + self.yo, z - zo + self.zo, a - ao + self.ao, b - bo + self.bo, c - bo + self.bo,
          u - uo + self.uo, v - vo + self.vo, w - wo + self.wo)
        self.xo = xo
        self.yo = yo
        self.zo = zo
        self.ao = ao
        self.bo = bo
        self.co = co
        self.uo = uo
        self.vo = vo
        self.wo = wo

    def set_spindle_rate(self, arg): pass
    def set_feed_rate(self, arg): self.feedrate = arg / 60.
    def select_plane(self, arg): pass

    def change_tool(self, pocket):
        self.first_move = True
        # keep track of current tooling so it can be associated with the various paths. this enables us to later
        # create display lists based on tooling
        self.current_tool = pocket
        # FIXME does defaultdict with list serve the correct purpose here? We used to add empty lists to each dict

    def straight_traverse(self, lineno, x, y, z, a, b, c, u, v, w):
        if self.suppress > 0: return
        l = self.rotate_and_translate(x, y, z, a, b, c, u, v, w)
        k = self.make_motion_key('traverse')
        if not self.first_move or self.exec_mode == gcode.PARSE_EXEC_PREP:
            self.tool_motions[k].append((lineno, self.lo, l, [self.xo, self.yo, self.zo]))
        self.lo = l

    def rigid_tap(self, lineno, x, y, z):
        if self.suppress > 0: return
        self.first_move = False
        l = self.rotate_and_translate(x,y,z,0,0,0,0,0,0)[:3]
        l += [self.lo[3], self.lo[4], self.lo[5],
               self.lo[6], self.lo[7], self.lo[8]]
        k = self.make_motion_key('straight_feed')
        self.tool_motions[k].append((lineno, self.lo, l, self.feedrate, [self.xo, self.yo, self.zo]))
#        self.dwells.append((self.lineno, self.colors['dwell'], x + self.offset_x, y + self.offset_y, z + self.offset_z, 0))
        self.tool_motions[k].append((lineno, l, self.lo, self.feedrate, [self.xo, self.yo, self.zo]))

    def arc_feed(self, *args):
        if self.suppress > 0: return
        self.first_move = False
        self.in_arc = True
        try:
            ArcsToSegmentsMixin.arc_feed(self, *args)
        finally:
            self.in_arc = False

    def straight_arcsegments(self, lineno, segs):
        self.first_move = False
        lo = self.lo
        feedrate = self.feedrate
        to = [self.xo, self.yo, self.zo]
        k = self.make_motion_key('arc_feed')
        for l in segs:
            self.tool_motions[k].append((lineno, lo, l, feedrate, to))
            lo = l
        self.lo = lo

    def straight_feed(self, lineno, x,y,z, a,b,c, u, v, w):
        if self.suppress > 0: return
        self.first_move = False
        l = self.rotate_and_translate(x,y,z,a,b,c,u,v,w)
        self.tool_motions[self.make_motion_key('straight_feed')].append((lineno, self.lo, l, self.feedrate, [self.xo, self.yo, self.zo]))
        self.lo = l

    # confusing, but I guess this is an alias for straight_feed above
    straight_probe = straight_feed
    # KLUDGE will complain if the endpoint is not within limits even though it doesn't matter
    straight_probe_to_limit = straight_feed

    def straight_feed_synched(self, lineno, x,y,z, a,b,c, u,v,w, s, fpr):
        """For spindle-synched straight feeds, also collect data needed to
        check if the commanded spindle rate and feed per revolution
        will violate any axis MAX_VELOCITY constraints"""
        if self.suppress > 0: return
        # save segment start and record straight feed segment
        lo = self.lo
        self.straight_feed(lineno, x,y,z, a,b,c, u, v, w)
        # record axis distances, spindle speed and feed per revolution
        delta = tuple([abs(i[0]-i[1]) for i in zip(lo, self.lo)])
        self.feed_synched.append((lineno, delta, s, fpr))

    def user_defined_function(self, i, p, q):
        if self.suppress > 0: return
        color = self.color_config.get_color('m1xx')
        self.dwells.append((self.lineno, color, self.lo[0], self.lo[1], self.lo[2], 0))

    def dwell(self, arg):
        if self.suppress > 0: return
        self.dwell_time += arg
        color = self.color_config.get_color('dwell' if self.exec_mode != gcode.PARSE_EXEC_PREP else 'leadin')
        self.dwells.append((self.lineno, color, self.lo[0], self.lo[1], self.lo[2], 0))

    def synch_position(self, x, y, z, a, b, c, u, v, w):
        #FIXME check for valid position here? This should ONLY be used to set up the machine initial position
        self.lo = (x, y, z, a, b, c, u, v, w)

    def set_tool_data_capsule(self, capsule):
        self.tool_data_capsule = capsule

    def add_tool_motion_key(self, motion_type, exec_mode, tool_number):
        self.tool_motion_types.add(MotionKey(motion_type, exec_mode, tool_number))

    def clear_tool_motion_keys(self):
        self.tool_motion_types = set()

    def highlight(self, lineno):
        #t0 = time.time()
        glLineWidth(3)
        c = self.color_config.get_color('selected')
        glColor3f(*c)
        coords = linuxcnc.draw_lines_by_property(self.tool_data_capsule, line_number=lineno)

        for line in self.dwells:
            if line[0] != lineno:
                continue
            self.draw_dwells([(line[0], c) + line[2:]], 2, 0)
            coords.append(line[2:5])
        glLineWidth(1)
        if coords:
            x = reduce(lambda x,y:x+y, [c[0] for c in coords]) / len(coords)
            y = reduce(lambda x,y:x+y, [c[1] for c in coords]) / len(coords)
            z = reduce(lambda x,y:x+y, [c[2] for c in coords]) / len(coords)
        else:
            x = (self.min_extents[0] + self.max_extents[0])/2
            y = (self.min_extents[1] + self.max_extents[1])/2
            z = (self.min_extents[2] + self.max_extents[2])/2

        #print "gremlin.highlight() call time: {}".format(time.time()-t0)
        return x, y, z

    def get_visible_tools(self):
        '''
        Returns a list of visible tool paths.  None means all tools are visible.
        '''
        return self.visible_tools

    def set_visible_tools(self, tool_list):
        '''
        Setting the list to None means 'show all tools'
        '''
        self.visible_tools = tool_list

    def make_motion_key(self, motion_type, exec_mode=None, tool_no=None):
        exec_mode = self.exec_mode if exec_mode is None else exec_mode
        tool_no = self.current_tool if tool_no is None else tool_no
        return MotionKey(motion_type, exec_mode, tool_no)

    def draw_rapids(self, for_selection):
        glEnable(GL_LINE_STIPPLE)
        for k in self.tool_motion_types:
            if k.motion_type != 'traverse':
                continue

            if self.visible_tools is None or k.tool_no in self.visible_tools:
                color_base_name = self.color_config.exec_mode_colormap.get(k.exec_mode, k.motion_type)
                self.colored_lines_by_type(color_base_name, k, for_selection)
        glDisable(GL_LINE_STIPPLE)

    def draw_nonrapids(self, for_selection):
        for k in self.tool_motion_types:
            if k.motion_type == 'traverse':
                continue

            if self.visible_tools is None or k.tool_no in self.visible_tools:
                color_base_name = self.color_config.exec_mode_colormap.get(k.exec_mode, k.motion_type)
                self.colored_lines_by_type(color_base_name, k, for_selection)

        glLineWidth(2)
        self.draw_dwells(self.dwells, self.color_config.get_alpha_or_default('dwell'), for_selection)
        glLineWidth(1)


def with_context(f):
    def inner(self, *args, **kw):
        self.activate()
        try:
            return f(self, *args, **kw)
        finally:
            self.deactivate()
    return inner


def with_context_swap(f):
    def inner(self, *args, **kw):
        self.activate()
        try:
            return f(self, *args, **kw)
        finally:
            self.swapbuffers()
            self.deactivate()
    return inner


class GlDrawColors:
    def __init__(self):
        self.default_alpha = 1. / 3.
        self.alphas = {
            'backplotprobing': 0.75,
            'backplotjog': 0.75,
            'lathetool': 0.10,
            'backplottoolchange': 0.25,
            'backplottraverse': 0.25,
            'overlay': 0.75,
            'tool': 0.20,
            'backplotfeed': 0.75,
            'backplotarc': 0.75,
            'skipped_segments': 0.25,
        }
        self.colors = {
            'traverse': (0.30, 0.50, 0.50),
            'traverse_xy': (0.30, 0.50, 0.50),
            'traverse_uv': (0.30, 0.50, 0.50),
            'backplotprobing': (0.63, 0.13, 0.94),
            'backplottraverse': (0.30, 0.50, 0.50),
            'label_ok': (1.00, 0.51, 0.53),
            'tool_diffuse': (0.60, 0.60, 0.60),
            'backplotfeed': (0.75, 0.25, 0.25),
            'back': (0.00, 0.00, 0.00),
            'axis_y': (0.20, 1.00, 0.20),
            'cone': (1.00, 1.00, 1.00),
            'cone_xy': (0.00, 1.00, 0.00),
            'cone_uv': (0.00, 0.00, 1.00),
            'axis_z': (0.20, 0.20, 1.00),
            'label_limit': (1.00, 0.21, 0.23),
            'backplotjog': (1.00, 1.00, 0.00),
            'selected': (0.00, 1.00, 1.00),
            'lathetool': (0.80, 0.80, 0.80),
            'dwell': (1.00, 0.50, 0.50),
            'overlay_foreground': (1.00, 1.00, 1.00),
            'overlay_background': (0.00, 0.00, 0.00),
            'straight_feed': (1.00, 1.00, 1.00),
            'rigid_tap': (1.0, 1.0, 0.8),
            'probe': (0.815, 0.565, 0.97),
            'straight_feed_xy': (0.20, 1.00, 0.20),
            'straight_feed_uv': (0.20, 0.20, 1.00),
            'small_origin': (0.00, 1.00, 1.00),
            'tool_ambient': (0.40, 0.40, 0.40),
            'backplottoolchange': (1.00, 0.65, 0.00),
            'backplotarc': (0.75, 0.25, 0.50),
            'm1xx': (0.50, 0.50, 1.00),
            'arc_feed': (1.00, 1.00, 1.00),
            'arc_feed_xy': (0.20, 1.00, 0.20),
            'arc_feed_uv': (0.20, 0.20, 1.00),
            'axis_x': (1.00, 0.20, 0.20),
            'grid': (0.15, 0.15, 0.15),
            'skipped_segments': (0.35, 0.35, 0.25),
            'leadin': (0.4, 1.0, 0.5),
            'work_offset': (.4, .45, .8),
            'axis_offset': (.4, .6, .8),
        }

        # NOTE: PARSE_EXEC_RUN is deliberately omitted here so that the lookup will fall back to the motion type
        self.exec_mode_colormap = {
            gcode.PARSE_EXEC_SEEK: 'skipped_segments',
            gcode.PARSE_EXEC_PREP: 'leadin',
        }

    def get_alpha_or_default(self, color):
        return self.alphas.get(color, self.default_alpha)

    def get_color(self, color):
        return self.colors[color]

    def gl_color_with_alpha(self, name):
        # NOTE: concatenates in to a 4-tuple of floats
        c = (self.colors.get(name, (1., 1., 1.)) + (self.get_alpha_or_default(name),))
        glColor4f(*c)

    def gl_color(self, name):
        glColor3f(*self.colors[name])

    def lkup_color(self, s):
        a = self.get_alpha_or_default(s)
        s = self.get_color(s)
        return [int(x * 255) for x in s + (a,)]

    def set_color(self, name, r, g, b, a=None):
        name = name.lower()
        if name not in self.colors:
            return False
        self.colors[name] = (float(r), float(g), float(b))
        if a:
            self.alphas[name] = float(a)
        return True


class GlCanonDraw(glnav.GlNavBase):

    def __init__(self, s, inifile, color_config, canon_inst=None):
        glnav.GlNavBase.__init__(self)
        self.hershey = None
        self.inifile = inifile
        self.color_config = color_config
        self.stat = s
        self.canon = canon_inst
        self._dlists = {}
        self.select_buffer_size = 100
        self.cached_tool = -1
        self.initialised = 0
        self._work_offsets_used = []
        self.x_limit_positive = float(inifile.find("AXIS_0", "MAX_LIMIT"))
        self.x_limit_negative = float(inifile.find("AXIS_0", "MIN_LIMIT"))
        self.y_limit_positive = float(inifile.find("AXIS_1", "MAX_LIMIT") or 0)
        self.y_limit_negative = float(inifile.find("AXIS_1", "MIN_LIMIT") or 0)
        self.z_limit_positive = float(inifile.find("AXIS_2", "MAX_LIMIT"))
        self.z_limit_negative = float(inifile.find("AXIS_2", "MIN_LIMIT"))
        self.highlight_line = None

        temp = inifile.find("DISPLAY", "LATHE")
        self.lathe_option = bool(temp == "1" or temp == "True" or temp == "true" )

        temp2 = inifile.find("DISPLAY", "GEOMETRY")
        self.rear_tp_lathe_option = bool(temp2 == "-XZ")

        self.foam_option = bool(inifile.find("DISPLAY", "FOAM"))
        self.geometry = None
        self.set_geometry()

        self.a_axis_wrapped = inifile.find("AXIS_3", "WRAPPED_ROTARY")
        self.b_axis_wrapped = inifile.find("AXIS_4", "WRAPPED_ROTARY")
        self.c_axis_wrapped = inifile.find("AXIS_5", "WRAPPED_ROTARY")

        live_axis_count = 0
        for i, j in enumerate("XYZABCUVW"):
            if self.stat.axis_mask & (1<<i) == 0:
                continue
            live_axis_count += 1
        self.num_joints = int(inifile.find("TRAJ", "JOINTS") or live_axis_count)

        # Now on by default, but can be turned off with ADMIN SET_PREVIEW_ALPHA
        self.program_alpha = True
        self.use_joints_mode = False
        self.use_commanded = True
        self.show_limits = True
        self.show_extents_option = True
        self.show_live_plot = True
        self.show_velocity = True
        self.metric_units = True
        self.show_program = True
        self.show_rapids = True
        self.use_relative = True
        self.show_tool = True
        self.show_dtg = True
        self.show_stat_overlay = False
        self.grid_size = 0.0
        self.show_offsets = True
        self.show_g53_origin = False

        self.current_view = 'z'
        self.font_base = None
        self.font_linespace = None
        self.font_charwidth = None

    # Abstract methods

    def _redraw(self):
        raise NotImplementedError

    def activate(self):
        raise NotImplementedError

    def update_glcanon(self, new_canon):
        self.canon = new_canon

    def get_geometry(self):
        return self.geometry

    def set_geometry(self, geometry_override=None):
        geom_spec = geometry_override or (self.inifile.find("DISPLAY", "GEOMETRY") or 'XYZ').upper()

        # KLUDGE check for special rendering cases here
        # TODO find a way to generalize this
        if geom_spec == 'XYZA':
            y0 = self.inifile.find("DISPLAY", "A_AXIS_Y_ORIGIN") or 0.0
            z0 = self.inifile.find("DISPLAY", "A_AXIS_Z_ORIGIN") or 0.0
            flipped = self.inifile.find("DISPLAY", "A_AXIS_FLIPPED") or 0.0
            linuxcnc.set_preview_kinematics(linuxcnc.KinsRotaryA, (float(y0), float(z0), float(flipped)))
            # For reference only, may remove this
            self.geometry = geom_spec
        else:
            # Reverse the provided order internally to match axis (not sure why)
            geometry_elems = re.split(" *(-?[XYZABCUVW])", geom_spec)
            geometry_reversed = ''.join(reversed(geometry_elems))
            linuxcnc.set_preview_kinematics(linuxcnc.KinsNaive, (geometry_reversed,))
            self.geometry = geometry_reversed
        print linuxcnc.get_kinematics_description()

    def realize(self):
        self.hershey = hershey.Hershey()
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
        self.basic_lighting()
        self.initialised = 1

    def set_canon(self, canon):
        self.canon = canon

    @with_context
    def basic_lighting(self):
        glLightfv(GL_LIGHT0, GL_POSITION, (1, -1, 1, 0))
        glLightfv(GL_LIGHT0, GL_AMBIENT, self.color_config.get_color('tool_ambient') + (0,))
        glLightfv(GL_LIGHT0, GL_DIFFUSE, self.color_config.get_color('tool_diffuse') + (0,))
        # Disable this since we weren't actually using it, and now with proper inheritance this override is called
        # glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (1,1,1,0))
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        glDepthFunc(GL_LESS)
        glEnable(GL_DEPTH_TEST)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

    def select(self, x, y_inv):
        if self.canon is None:
            return
        # t0 = time.time()
        # invert y now
        vport = glGetIntegerv(GL_VIEWPORT)
        y = vport[3] - y_inv
        # print "viewport is {}, with pick location ({}, {})".format(vport, x, y))
        glMatrixMode(GL_MODELVIEW)

        clear_color = [0, 0, 0, 0]
        glClearColor(*clear_color)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        # This is mostly for insurance, doesn't seem to have an effect currently
        glForceColorPassthrough()

        t0_tmp = time.time()
        if self.get_show_rapids():
            glCallList(self.dlist('select_rapids', gen=lambda n: self.make_rapids_list(n, True)))
        glCallList(self.dlist('select_norapids', gen=lambda n: self.make_nonrapids_list(n, True)))
        # print "glCallList(s) call time: {}".format(time.time()-t0_tmp)
        glFlush()

        # This brute-force search is inefficient but it's fast enough
        search_radius = 3
        x = int(x)
        y = int(y)
        found = defaultdict(int)

        x_min = max(x - search_radius, vport[0])
        x_max = min(x + search_radius, vport[2])
        y_min = max(y - search_radius, vport[1])
        y_max = min(y + search_radius, vport[3])
        # print "searching vport subsection {}".format([x_min, y_min, x_max, y_max])
        for x_test in range(x_min, x_max +1):
            for y_test in range(y_min, y_max + 1):
                px_data = glReadPixels(int(x_test), int(y_test), 1, 1, GL_RGB, GL_UNSIGNED_BYTE)
                rgba = [struct.unpack("@b", px_elem) for px_elem in px_data]
                # print "pixel rgba is {}".format(rgba)
                picked_line_no = struct.unpack("<I", px_data[0:3]+'\0')[0] if rgba != clear_color else None
                # Weight by maximum pixel distance
                w = (2*search_radius + 1) ** 2 / max(abs(np.array([x_test-x, y_test-y, 1])))
                found[picked_line_no] += float(picked_line_no > 0) * w
                # print "pixel at {}, {} is line {} with weight {}".format(x_test, y_test, picked_line_no, found[picked_line_no])

        most_hits = max(found.iteritems(), key=operator.itemgetter(1))[0]
        if most_hits and found[most_hits] > 0:
            # print "found best candidate {} with weight {}".format(most_hits, found[most_hits])
            self.set_highlight_line(most_hits)
        else:
            self.set_highlight_line(None)
        # print "glcanon.select() call time: {}".format(time.time()-t0)

    def dlist(self, name, n=1, gen=lambda n: None):
        if name not in self._dlists:
            base = glGenLists(n)
            self._dlists[name] = base, n
            gen(base)
        return self._dlists[name][0]

    def stale_dlist(self, name):
        if name not in self._dlists: return
        base, count = self._dlists.pop(name)
        glDeleteLists(base, count)

    def __del__(self):
        for base, count in self._dlists.values():
            glDeleteLists(base, count)

    def set_current_line(self, line): pass
    def set_highlight_line(self, line):
        if line and line == self.get_highlight_line():
            return
        self.highlight_line = line
        self.stale_dlist('highlight')
        if self.canon is None:
            self.set_centerpoint(0, 0, 0)
            return
        elif line is None:
            x = (self.canon.min_extents[0] + self.canon.max_extents[0]) / 2
            y = (self.canon.min_extents[1] + self.canon.max_extents[1]) / 2
            z = (self.canon.min_extents[2] + self.canon.max_extents[2]) / 2
            self.set_centerpoint(x, y, z)
            return

        with gl_new_list(self.dlist('highlight')), gl_push_matrix():
            glTranslatef(0, 0, self.get_foam_z())
            if self.foam_option:
                self.set_geometry("XY")
            x, y, z = self.canon.highlight(line)

            if self.foam_option:
                glTranslatef(0, 0, self.get_foam_w()-self.get_foam_z())
                self.set_geometry("UV")
                u, v, w = self.canon.highlight(line)
                x = (x+u)/2
                y = (y+v)/2
                z = (self.get_foam_z() + self.get_foam_w())/2
        self.set_centerpoint(x, y, z)

    @staticmethod
    def gl_look_at_z(distance):
        gluLookAt(
            0, 0, distance,
            0, 0, 0,
            0., 1., 0.)

    @with_context_swap
    def redraw_view(self):
        if not self.initialised:
            return

        # Define user view of the scene
        w = self.winfo_width()
        h = self.winfo_height()
        glViewport(0, 0, w, h)

        # Clear the background and depth buffer.
        glClearColor(*(self.color_config.get_color('back') + (0,)))
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()

        if self.perspective_view:
            gluPerspective(self.fovy, float(w) / float(h), self.near, self.far + self.distance)
            self.gl_look_at_z(self.distance)
        else:
            horiz_clip = (abs(self.distance or 1)) ** .55555
            vert_clip = horiz_clip * h / w
            near_clip = -1000
            far_clip = 1000
            glOrtho(-horiz_clip, horiz_clip, -vert_clip, vert_clip, near_clip, far_clip)
            self.gl_look_at_z(1.0)

        glMatrixMode(GL_MODELVIEW)
        try:
            world_tf, lp, pos6 = self.get_logger_world_transform()
            pos6 = pos6 or (0,) * 6
            self.swap_world_view_transform(world_tf)
            with gl_push_matrix():
                self.redraw_scene(lp, pos6)
        finally:
            glFlush()

    def color_limit(self, cond):
        if cond:
            glColor3f(*self.color_config.get_color('label_limit'))
        else:
            glColor3f(*self.color_config.get_color('label_ok'))
        return cond

    def draw_program_extents(self):
        s = self.stat
        g = self.canon
        if g is None: return

        # Dimensions
        x,y,z,p = 0,1,2,3
        view = self.get_view()
        is_metric = self.get_show_metric()
        dimscale = is_metric and 25.4 or 1.0
        fmt = is_metric and "%.1f" or "%.2f"
        dfmt = is_metric and "Ds%.1f" or "Ds%.2f"

        machine_limit_min, machine_limit_max = self.soft_limits()

        pullback = max(g.max_extents[x] - g.min_extents[x],
                       g.max_extents[y] - g.min_extents[y],
                       g.max_extents[z] - g.min_extents[z],
                       2 ) * .1

        dashwidth = pullback/4
        charsize = dashwidth * 1.5
        halfchar = charsize * .5

        if view == z or view == p:
            z_pos = g.min_extents[z]
            zdashwidth = 0
        else:
            z_pos = g.min_extents[z] - pullback
            zdashwidth = dashwidth

        self.color_limit(0)
        glBegin(GL_LINES)

        # x dimension
        if view != x and g.max_extents[x] > g.min_extents[x]:
            if self.rear_tp_lathe_option:
                x_dir = -1
            else:
                x_dir = 1

            y_pos = g.min_extents[y] - pullback
            glVertex3f(x_dir * g.min_extents[x], y_pos, z_pos)
            glVertex3f(x_dir * g.max_extents[x], y_pos, z_pos)

            glVertex3f(x_dir * g.min_extents[x], y_pos - dashwidth, z_pos - zdashwidth)
            glVertex3f(x_dir * g.min_extents[x], y_pos + dashwidth, z_pos + zdashwidth)

            glVertex3f(x_dir * g.max_extents[x], y_pos - dashwidth, z_pos - zdashwidth)
            glVertex3f(x_dir * g.max_extents[x], y_pos + dashwidth, z_pos + zdashwidth)

        # y dimension
        if view != y and g.max_extents[y] > g.min_extents[y]:
            x_pos = g.min_extents[x] - pullback
            glVertex3f(x_pos, g.min_extents[y], z_pos)
            glVertex3f(x_pos, g.max_extents[y], z_pos)

            glVertex3f(x_pos - dashwidth, g.min_extents[y], z_pos - zdashwidth)
            glVertex3f(x_pos + dashwidth, g.min_extents[y], z_pos + zdashwidth)

            glVertex3f(x_pos - dashwidth, g.max_extents[y], z_pos - zdashwidth)
            glVertex3f(x_pos + dashwidth, g.max_extents[y], z_pos + zdashwidth)

        # z dimension
        if view != z and g.max_extents[z] > g.min_extents[z]:
            if self.rear_tp_lathe_option:
                x_dir = -1
            else:
                x_dir = 1

            x_pos = g.min_extents[x] - pullback
            y_pos = g.min_extents[y] - pullback
            glVertex3f(x_dir * x_pos, y_pos, g.min_extents[z])
            glVertex3f(x_dir * x_pos, y_pos, g.max_extents[z])

            glVertex3f(x_dir * x_pos - dashwidth, y_pos - zdashwidth, g.min_extents[z])
            glVertex3f(x_dir * x_pos + dashwidth, y_pos + zdashwidth, g.min_extents[z])

            glVertex3f(x_dir * x_pos - dashwidth, y_pos - zdashwidth, g.max_extents[z])
            glVertex3f(x_dir * x_pos + dashwidth, y_pos + zdashwidth, g.max_extents[z])

        glEnd()

        # Labels
        if self.get_show_relative():
            offset = self.to_internal_units(s.g5x_offset + s.g92_offset)
        else:
            offset = 0, 0, 0

        if self.rear_tp_lathe_option:
            x_dir = -1
        else:
            x_dir = 1

        # ### plot Z extent labels ###
        if view != z and g.max_extents[z] > g.min_extents[z]:

            if view == x:
                x_pos = g.min_extents[x] - pullback
                y_pos = g.min_extents[y] - 6.0*dashwidth

            else:  # view must be y then
                x_pos = x_dir * (g.min_extents[x] - 6.0*dashwidth)
                y_pos = g.min_extents[y] - pullback

            bbox = self.color_limit(g.min_extents_notool[z] < machine_limit_min[z])
            glPushMatrix()
            f = fmt % ((g.min_extents[z]-offset[z]) * dimscale)
            glTranslatef(x_pos, y_pos, g.min_extents[z] - halfchar)
            glScalef(charsize, charsize, charsize)
            glRotatef(x_dir * -90, 0, 1, 0)
            glRotatef(-90, 0, 0, 1)
            if view != x:
                glRotatef(-90, 0, 1, 0)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            bbox = self.color_limit(g.max_extents_notool[z] > machine_limit_max[z])
            glPushMatrix()
            f = fmt % ((g.max_extents[z]-offset[z]) * dimscale)
            glTranslatef(x_pos, y_pos, g.max_extents[z] - halfchar)
            glScalef(charsize, charsize, charsize)
            glRotatef(x_dir * -90, 0, 1, 0)
            glRotatef(-90, 0, 0, 1)
            if view != x:
                glRotatef(-90, 0, 1, 0)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            # create Z extnet distance value and ready for plotting
            self.color_limit(0)
            glPushMatrix()
            f = fmt % ((g.max_extents[z] - g.min_extents[z]) * dimscale)
            glTranslatef(x_pos, y_pos, (g.max_extents[z] + g.min_extents[z])/2)
            glScalef(charsize, charsize, charsize)
            if view != x:
                glRotatef(-90, 0, 0, 1)
            glRotatef(-90, 0, 1, 0)
            self.hershey.plot_string(f, .5, bbox)
            glPopMatrix()

        # ### plot Y extent labels ###
        if view != y and g.max_extents[y] > g.min_extents[y]:
            x_pos = g.min_extents[x] - 6.0*dashwidth

            bbox = self.color_limit(g.min_extents_notool[y] < machine_limit_min[y])
            glPushMatrix()
            f = fmt % ((g.min_extents[y] - offset[y]) * dimscale)
            glTranslatef(x_pos, g.min_extents[y] + halfchar, z_pos)
            glRotatef(-90, 0, 0, 1)
            glRotatef(-90, 0, 0, 1)
            if view == x:
                glRotatef(90, 0, 1, 0)
                glTranslatef(dashwidth*1.5, 0, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            bbox = self.color_limit(g.max_extents_notool[y] > machine_limit_max[y])
            glPushMatrix()
            f = fmt % ((g.max_extents[y] - offset[y]) * dimscale)
            glTranslatef(x_pos, g.max_extents[y] + halfchar, z_pos)
            glRotatef(-90, 0, 0, 1)
            glRotatef(-90, 0, 0, 1)
            if view == x:
                glRotatef(90, 0, 1, 0)
                glTranslatef(dashwidth*1.5, 0, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            self.color_limit(0)
            glPushMatrix()
            f = fmt % ((g.max_extents[y] - g.min_extents[y]) * dimscale)

            glTranslatef(x_pos, (g.max_extents[y] + g.min_extents[y])/2,
                        z_pos)
            glRotatef(-90, 0, 0, 1)
            if view == x:
                glRotatef(-90, 1, 0, 0)
                glTranslatef(0, halfchar, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, .5)
            glPopMatrix()

        # ### plot X extent labels ###
        if view != x and g.max_extents[x] > g.min_extents[x]:

            y_pos = g.min_extents[y] - 6.0*dashwidth

            bbox = self.color_limit(g.min_extents_notool[x] < machine_limit_min[x])
            glPushMatrix()

            if self.rear_tp_lathe_option:
                f = dfmt % ((g.min_extents[x] - offset[x]) * dimscale * 2)
            else:
                f = fmt % ((g.min_extents[x] - offset[x]) * dimscale)

            glTranslatef(x_dir * (g.min_extents[x] - halfchar), y_pos, z_pos)
            glRotatef(-90, 0, 0, 1)
            if view == y:
                glRotatef(90, 0, 1, 0)
                glTranslatef(dashwidth*1.5, 0, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            bbox = self.color_limit(g.max_extents_notool[x] > machine_limit_max[x])
            glPushMatrix()

            if self.rear_tp_lathe_option:
                f = dfmt % ((g.max_extents[x] - offset[x]) * dimscale * 2)
            else:
                f = fmt % ((g.max_extents[x] - offset[x]) * dimscale)

            glTranslatef(x_dir * (g.max_extents[x] - halfchar), y_pos, z_pos)
            glRotatef(-90, 0, 0, 1)
            if view == y:
                glRotatef(90, 0, 1, 0)
                glTranslatef(dashwidth*1.5, 0, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, 0, bbox)
            glPopMatrix()

            # create X extnet distance value and ready for plotting
            self.color_limit(0)
            glPushMatrix()
            f = fmt % ((g.max_extents[x] - g.min_extents[x]) * dimscale)

            glTranslatef(x_dir * (g.max_extents[x] + g.min_extents[x])/2, y_pos,
                        z_pos)
            if view == y:
                glRotatef(-90, 1, 0, 0)
                glTranslatef(0, halfchar, 0)
            glScalef(charsize, charsize, charsize)
            self.hershey.plot_string(f, .5)
            glPopMatrix()

    def to_internal_linear_unit(self, v, unit=None):
        if unit is None:
            unit = self.stat.linear_units
        lu = (unit or 1) * 25.4
        return v/lu

    def to_internal_units(self, pos9, unit=None):
        if unit is None:
            unit = self.stat.linear_units
        lu = (unit or 1) * 25.4

        unit_scales = (lu, lu, lu, 1, 1, 1, lu, lu, lu)
        return [a/b for a, b in zip(pos9, unit_scales)]

    @staticmethod
    def mask_bignum(pos9):
        return [0 if abs(x) > 1e30 else x for x in pos9]

    def soft_limits(self):
        min_limits, max_limits = self.stat.axis_limits
        return (
            self.to_internal_units(self.mask_bignum(min_limits))[:3],
            self.to_internal_units(self.mask_bignum(max_limits))[:3],
        )

    def get_foam_z(self):
        """
        Assumed to be zero for single-point cutters, this describes the offset for the bottom of a wire-style cutter (foam, wire EDM, water jet?).
        """
        if self.canon:
            return self.canon.foam_z
        return 0

    def get_foam_w(self):
        """
        The offset for the top of a wire-style cutter (foam, wire EDM, water jet?)
        """
        if self.canon: return self.canon.foam_w
        return 1.5

    def get_grid(self):
        if self.canon and self.canon.grid: return self.canon.grid
        return 5./25.4

    def comp(self, (sx, sy), (cx, cy)):
        return -(sx*cx + sy*cy) / (sx*sx + sy*sy)

    def param(self, (x1, y1), (dx1, dy1), (x3, y3), (dx3, dy3)):
        den = (dy3)*(dx1) - (dx3)*(dy1)
        if den == 0: return 0
        num = (dx3)*(y1-y3) - (dy3)*(x1-x3)
        return num * 1. / den

    def draw_grid_lines(self, space, (ox, oy), (dx, dy), lim_min, lim_max,
            inverse_permutation, line_dir):
        # draw a series of line segments of the form
        #   dx(x-ox) + dy(y-oy) + k*space = 0
        # for integers k that intersect the AABB [lim_min, lim_max]
        lim_pts = [
                (lim_min[0], lim_min[1]),
                (lim_max[0], lim_min[1]),
                (lim_min[0], lim_max[1]),
                (lim_max[0], lim_max[1])]
        od = self.comp((dy, -dx), (ox, oy))
        d0, d1 = minmax(*(self.comp((dy, -dx), i)-od for i in lim_pts))
        k0 = int(math.ceil(d0/space))
        k1 = int(math.floor(d1/space))
        delta = (dx, dy)

        s = self.stat

        if self.is_rear_tool_post_lathe():
            x_dir = -1
            machine_limit_min, machine_limit_max = self.soft_limits()

            if line_dir == 1:
                grid_offset = 0
                grid_count = 0
                while (-d1 + grid_offset) < (machine_limit_max[2] - s.tool_offset[2]):
                    if (-d1 + grid_offset) > (machine_limit_min[2] - s.tool_offset[2]):
                        glVertex3f((x_dir * (machine_limit_min[0] - s.tool_offset[0])), 0.0, (-d1 + grid_offset))
                        glVertex3f((x_dir * (machine_limit_max[0] - s.tool_offset[0])), 0.0, (-d1 + grid_offset))
                    grid_offset = grid_count * space
                    grid_count += 1
                    if grid_count > 1000:
                        break

                grid_offset = 0
                grid_count = 0
                while (-d1 + grid_offset) > (machine_limit_min[2] - s.tool_offset[2]):
                    if (-d1 + grid_offset) < (machine_limit_max[2] - s.tool_offset[2]):
                        glVertex3f((x_dir * (machine_limit_min[0] - s.tool_offset[0])), 0.0, (-d1 + grid_offset))
                        glVertex3f((x_dir * (machine_limit_max[0] - s.tool_offset[0])), 0.0, (-d1 + grid_offset))
                    grid_offset = -grid_count * space
                    grid_count += 1
                    if grid_count > 1000:
                        break

            elif line_dir == 0:
                grid_offset = 0
                grid_count = 0
                while (-d1 + grid_offset) < (machine_limit_max[0] - s.tool_offset[0]):
                    if (-d1 + grid_offset) > (machine_limit_min[0] - s.tool_offset[0]):
                        glVertex3f((x_dir * (-d1 + grid_offset)), 0.0, (machine_limit_min[2] - s.tool_offset[2]))
                        glVertex3f((x_dir * (-d1 + grid_offset)), 0.0, (machine_limit_max[2] - s.tool_offset[2]))
                    grid_offset = grid_count * space
                    grid_count += 1
                    if grid_count > 1000:
                        break

                grid_offset = 0
                grid_count = 0
                while (-d1 + grid_offset) > (machine_limit_min[0] - s.tool_offset[0]):
                    if (-d1 + grid_offset) < (machine_limit_max[0] - s.tool_offset[0]):
                        glVertex3f((x_dir * (-d1 + grid_offset)), 0.0, (machine_limit_min[2] - s.tool_offset[2]))
                        glVertex3f((x_dir * (-d1 + grid_offset)), 0.0, (machine_limit_max[2] - s.tool_offset[2]))
                    grid_offset = -grid_count * space
                    grid_count += 1
                    if grid_count > 1000:
                        break
        else:
            for k in range(k0, k1+1):
                d = k*space
                # Now we're drawing the line dx(x-ox) + dx(y-oy) + d = 0
                p0 = (ox - dy * d, oy + dx * d)
                # which is the same as the line p0 + u * delta

                # but we only want the part that's inside the box lim_pts...
                if dx and dy:
                    times = [
                            self.param(p0, delta, lim_min[:2], (0, 1)),
                            self.param(p0, delta, lim_min[:2], (1, 0)),
                            self.param(p0, delta, lim_max[:2], (0, 1)),
                            self.param(p0, delta, lim_max[:2], (1, 0))]
                    times.sort()
                    t0, t1 = times[1], times[2] # Take the middle two times
                elif dx:
                    times = [
                            self.param(p0, delta, lim_min[:2], (0, 1)),
                            self.param(p0, delta, lim_max[:2], (0, 1))]
                    times.sort()
                    t0, t1 = times[0], times[1] # Take the only two times
                else:
                    times = [
                            self.param(p0, delta, lim_min[:2], (1, 0)),
                            self.param(p0, delta, lim_max[:2], (1, 0))]
                    times.sort()
                    t0, t1 = times[0], times[1] # Take the only two times
                x0, y0 = p0[0] + delta[0]*t0, p0[1] + delta[1]*t0
                x1, y1 = p0[0] + delta[0]*t1, p0[1] + delta[1]*t1
                xm, ym = (x0+x1)/2, (y0+y1)/2
                # The computation of k0 and k1 above should mean that
                # the lines are always in the limits, but I observed
                # that this wasn't always the case...
                #if xm < lim_min[0] or xm > lim_max[0]: continue
                #if ym < lim_min[1] or ym > lim_max[1]: continue

                view = self.get_view()
                #if self.is_rear_tool_post_lathe():
                #    glVertex3f(*inverse_permutation((x0 - s.tool_offset[2], -y0 + s.tool_offset[0], lim_min[2])))
                #    glVertex3f(*inverse_permutation((x1 - s.tool_offset[2], -y1 + s.tool_offset[0], lim_min[2])))
                #elif view == 2:
                if view == 2:
                    glVertex3f(*inverse_permutation((x0, y0, lim_min[2])))
                    glVertex3f(*inverse_permutation((x1, y1, lim_min[2])))
                else:
                    glVertex3f(*inverse_permutation((x0, y0, lim_min[2])))
                    glVertex3f(*inverse_permutation((x1, y1, lim_min[2])))

                """
                if self.is_rear_tool_post_lathe():
                    x_dir = -1
                else:
                    x_dir = 1
                view = self.get_view()
                if view == 2:
                    glVertex3f(*inverse_permutation((x0, x_dir * y0, lim_min[2] - s.tool_offset[2])))
                    glVertex3f(*inverse_permutation((x1, x_dir * y1, lim_min[2] - s.tool_offset[2])))
                else:
                    glVertex3f(*inverse_permutation((x0 - s.tool_offset[2], x_dir * y0, lim_min[2])))
                    glVertex3f(*inverse_permutation((x1 - s.tool_offset[2], x_dir * y1, lim_min[2])))
                """

    def draw_grid_permuted(self, rotation, permutation, inverse_permutation, draw_at_top=False):
        grid_size = self.get_grid_size()
        if not grid_size:
            return

        glLineWidth(1)
        glColor3f(*self.color_config.get_color('grid'))

        s = self.stat
        tlo_orig = np.array(self.to_internal_units(s.tool_offset)[:3])
        tlo_permuted = permutation(self.to_internal_units(s.tool_offset)[:3])
        g5x_offset = permutation(self.to_internal_units(s.g5x_offset)[:3])[:2]
        g92_offset = permutation(self.to_internal_units(s.g92_offset)[:3])[:2]

        lim_min, lim_max = self.soft_limits()
        lim_min = np.array(permutation(lim_min))
        lim_max = np.array(permutation(lim_max))

        lim_min -= tlo_permuted
        lim_max -= tlo_permuted

        if self.get_show_relative():
            cos_rot = math.cos(rotation)
            sin_rot = math.sin(rotation)
            offset = (
                    g5x_offset[0] + g92_offset[0] * cos_rot
                                  - g92_offset[1] * sin_rot,
                    g5x_offset[1] + g92_offset[0] * sin_rot
                                  + g92_offset[1] * cos_rot)
        else:
            offset = 0., 0.
            cos_rot = 1.
            sin_rot = 0.
        glDepthMask(False)
        glBegin(GL_LINES)
        self.draw_grid_lines(grid_size, offset, (cos_rot, sin_rot),
                lim_min, lim_max, inverse_permutation, 0)
        self.draw_grid_lines(grid_size, offset, (sin_rot, -cos_rot),
                lim_min, lim_max, inverse_permutation, 1)
        glEnd()
        glDepthMask(True)

        # draw grid size label
        # TODO maybe have the label follow the origin?
        x,y,z,p = 0,1,2,3
        view = self.get_view()
        if view == p: return

        grid_size=self.get_grid_size()
        if self.get_show_metric():  # the 's' is a space for hershey.py
            grid_string = '%smm' % int(grid_size * 25.4)
        else:
            grid_string = '%ssin.' % grid_size

        with gl_push_matrix():
            glColor3f(1.0, 1.0, 1.0)
            glTranslatef(*(-np.array(tlo_orig)))
            if view == z:
                glTranslatef(0.2, 0.2, 0.0)
            elif view == y:
                glRotatef(90, 1, 0, 0)
                glTranslatef(0.3, -0.5, 0.0)
            elif view == x:
                glRotatef(90, 0, 1, 0)
                glRotatef(90, 0, 0, 1)
                glTranslatef(0.3, -0.5, 0.0)
            else:
                pass
            glScalef(0.3, 0.3, 0.3)
            self.hershey.plot_string(grid_string, 0.1)


    def draw_grid(self):
        x,y,z,p = 0,1,2,3
        view = self.get_view()
        if view == p:
            return
        rotation = math.radians(self.stat.rotation_xy % 90)
        if rotation != 0 and view != z and self.get_show_relative():
            return
        permutations = [
                lambda (x, y, z): np.array((z, y, x)),  # YZ X
                lambda (x, y, z): np.array((z, x, y)),  # ZX Y
                lambda (x, y, z): np.array((x, y, z)),  # XY Z
        ]
        inverse_permutations = [
                lambda (z, y, x): np.array((x, y, z)),  # YZ X
                lambda (z, x, y): np.array((x, y, z)),  # ZX Y
                lambda (x, y, z): np.array((x, y, z)),  # XY Z
        ]
        self.draw_grid_permuted(rotation, permutations[view],
                inverse_permutations[view])

    def redraw_scene(self, last_position=None, last_tpos=None):
        t0 = time.time()

        # Get needed data from status after poll
        self.stat.poll()
        machine_limit_min, machine_limit_max = self.soft_limits()
        tool_offset = self.stat.tool_offset
        units_per_inch = 1 / ((self.stat.linear_units or 1) * 25.4)
        g5x_index = self.stat.g5x_index
        g5x_offset = self.to_internal_units(self.stat.g5x_offset)[:3]
        if self.is_rear_tool_post_lathe():
            g5x_offset[0] *= -1
        rotation_xy = self.stat.rotation_xy
        g92_offset = self.to_internal_units(self.stat.g92_offset)[:3]

        glDisable(GL_LIGHTING) # Can't light lines, so only enable this for specific 3D geometry
        glMatrixMode(GL_MODELVIEW)

        # Go through each display option and draw any view elements needed
        self.draw_grid()

        if self.get_show_program():
            if self.get_program_alpha():
                glDisable(GL_DEPTH_TEST)
                glEnable(GL_BLEND)
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
            if self.get_show_rapids():
                glCallList(self.dlist('program_rapids', gen=self.make_rapids_list))
            glCallList(self.dlist('program_norapids', gen=self.make_nonrapids_list))
            glCallList(self.dlist('highlight', gen=self.make_highlight_list))

            if self.get_program_alpha():
                glDisable(GL_BLEND)
                glEnable(GL_DEPTH_TEST)

            if self.get_show_extents():
                self.draw_program_extents()

        if self.get_show_live_plot() or self.get_show_program():
            self.draw_coordinate_systems(g5x_index, g5x_offset, g92_offset, rotation_xy)

        if self.get_show_limits():
            self.draw_limit_box(machine_limit_max, machine_limit_min, last_position, tool_offset)

        self.draw_axis_chain(last_position)

        if self.get_show_live_plot():
            self.draw_live_plot_all(units_per_inch)

        if self.get_show_tool():
            self.draw_tool(last_tpos)

        if self.get_show_stat_overlay():
            self.draw_stat_overlay()

        t1 = time.time()
        dt_ms = 1000. * (t1 - t0)
        dt_cutoff_ms = 10.
        if dt_ms > dt_cutoff_ms:
            print "glcanon.redraw() call exceeded {} ms: {:0.3f} ms".format(dt_cutoff_ms, dt_ms)

    def draw_coordinate_systems(self, g5x_index, g5x_offset, g92_offset, rotation_xy):
        glPushMatrix()
        if self.get_show_relative() and (g5x_offset[0] or g5x_offset[1] or g5x_offset[2] or
                                         g92_offset[0] or g92_offset[1] or g92_offset[2] or
                                         rotation_xy):

            if self.show_g53_origin:  # plot machine zero
                glCallList(self.dlist('draw_small_origin', gen=self.draw_small_origin))

            if self.get_show_work_offset() and (g5x_offset[0] or g5x_offset[1] or g5x_offset[2]):
                self.draw_work_offset_viz(g5x_offset, g5x_index)
            # Translate into work offset / rotated coordinates
            glTranslatef(*g5x_offset)
            glRotatef(rotation_xy, 0, 0, 1)

            if self.get_show_axis_offset() and (g92_offset[0] or g92_offset[1] or g92_offset[2]):
                self.draw_axis_offset_viz(g92_offset)

            # Translate into local coordinates (in preparation for drawing the offset axes)
            glTranslatef(*g92_offset)
        # "Foam" z coordinate offset is zero on a non-wire machine
        glTranslatef(0, 0, self.get_foam_z())
        glCallList(self.dlist(('axes', self.get_view()), gen=self.make_local_axes))
        if self.foam_option:
            glTranslatef(0, 0, self.get_foam_w() - self.get_foam_z())
            uwalist = self.dlist(('axes_uw', self.get_view()), gen=lambda n: self.make_local_axes(n, 'UVW'))
            glCallList(uwalist)
        glPopMatrix()

    def draw_work_offset_viz(self, g5x_offset, g5x_index):
        # plot offset crosshair
        self.color_config.gl_color_with_alpha('work_offset')
        glLineStipple(1, 0x1C47)
        glEnable(GL_LINE_STIPPLE)
        glBegin(GL_LINES)
        glVertex3f(0, 0, 0)
        glVertex3f(*g5x_offset)
        glEnd()
        glDisable(GL_LINE_STIPPLE)
        # plot offset label
        i = g5x_index
        if i < 7:
            label = "G5%d" % (i + 3)
        elif i < 10:
            label = "G59.%d" % (i - 6)
        else:
            label = "G54.1 P{}".format(i)
        glPushMatrix()
        glScalef(0.2, 0.2, 0.2)
        if self.is_lathe():
            g5xrot = math.atan2(g5x_offset[0], -g5x_offset[2])
            glRotatef(90, 1, 0, 0)
            glRotatef(-90, 0, 0, 1)
        else:
            g5xrot = math.atan2(g5x_offset[1], g5x_offset[0])
        glRotatef(math.degrees(g5xrot), 0, 0, 1)
        glTranslatef(0.5, 0.5, 0)
        self.hershey.plot_string(label, 0.1)
        glPopMatrix()

    def draw_axis_offset_viz(self, g92_offset):
        self.color_config.gl_color_with_alpha('axis_offset')
        glLineStipple(1, 0x1C47)
        glEnable(GL_LINE_STIPPLE)
        glBegin(GL_LINES)
        glVertex3f(0, 0, 0)
        # g92_offset[0] *= -1 if rear tool lathe?
        glVertex3f(*g92_offset)
        glEnd()
        glDisable(GL_LINE_STIPPLE)
        self.gl_draw_g92_label(g92_offset)

    def draw_limit_box(self, machine_limit_max, machine_limit_min, position, tool_offset):
        # Chris Radek commited a 2 line change
        # https://github.com/LinuxCNC/linuxcnc/commit/b98597b97b8a777325bafb9e7eab18f4c8305077
        #
        # The entire limit box drawing was effected (see the the matching commented out
        # glTranslatef() and the end of this block which attempts to restore the previous matrix.
        # (not sure why he didn't just use glPushMatrix/glPopMatrix also)
        #
        # The change installs an identity translation matrix which effectively shifts all limit
        # box vertices negatively by the tool offset. Most visible in Z of course on the mill.
        #
        # Do not understand how he thought this was helping, the logic doesn't seem to match his
        # commit comment (my theory is its somehow effective in Axis so it felt right to him?).
        #
        # glTranslatef(*[-x for x in self.to_internal_units(s.tool_offset)[:3]])
        glLineWidth(1)
        glLineStipple(1, 0x1111)
        glEnable(GL_LINE_STIPPLE)
        tool_offset_xyz = np.array(tool_offset[0:3])
        machine_limit_xyz_min_tool = np.array(machine_limit_min[0:3]) - tool_offset_xyz
        machine_limit_xyz_max_tool = np.array(machine_limit_max[0:3]) - tool_offset_xyz
        if self.is_rear_tool_post_lathe():
            # Flip the draw X axis orientation and tweak the color for the rear TP
            machine_limit_xyz_min_tool[0] *= -1.
            machine_limit_xyz_max_tool[0] *= -1.
            glColor3f(0.0, 1.0, 0.0)
        else:
            # Normal limit box color
            glColor3f(0.3, 0.5, 1.0)
        linuxcnc.draw_oriented_limit_box(machine_limit_xyz_min_tool, machine_limit_xyz_max_tool, position)
        glDisable(GL_LINE_STIPPLE)
        glLineStipple(2, 0x5555)
        # See comment above at start of limit box drawing for why this is commented out.
        # glTranslatef(*self.to_internal_units(s.tool_offset)[:3])

    def draw_axis_chain(self, position):
        glLineWidth(1)
        glLineStipple(4, 0x9999)
        glEnable(GL_LINE_STIPPLE)
        linuxcnc.draw_oriented_axis_chain(position)
        glDisable(GL_LINE_STIPPLE)
        glLineStipple(2, 0x5555)

    def draw_live_plot_all(self, units_per_inch):
        glDepthFunc(GL_LEQUAL)
        glLineWidth(3)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        glEnable(GL_BLEND)
        glPushMatrix()
        glScalef(units_per_inch, units_per_inch, units_per_inch)
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        # Draws live plot slightly above the preview path in the user's view
        glTranslatef(0, 0, .003)
        self.draw_live_plot_lines()
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glPopMatrix()
        glDisable(GL_BLEND)
        glLineWidth(1)
        glDepthFunc(GL_LESS)

    def draw_tool(self, pos6):
        rx, ry, rz = pos6[3:6]
        pos = self.to_internal_units(pos6)[:3]
        if self.foam_option:
            self.draw_foamwire_tool(pos, rx, ry)
        else:
            self.draw_single_tool(pos, rx, ry, rz)

    def draw_single_tool(self, pos, rx, ry, rz):
        with gl_push_matrix():
            glTranslatef(*pos)
            # FIXME not sure if the extrinsic / intrinsic order is right here
            glRotatef(rz * 180./np.pi, 0, 0, 1)
            glRotatef(ry * 180./np.pi, 0, 1, 0)
            glRotatef(rx * 180./np.pi, 1, 0, 0)
            glEnable(GL_BLEND)
            glEnable(GL_CULL_FACE)
            glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA)
            current_tool = self.get_current_tool()
            if current_tool is None or current_tool.diameter == 0:
                cone_scale = self.find_tool_placeholder_scale()
                if self.is_lathe():
                    glRotatef(90, 0, 1, 0)
                cone = self.dlist("cone", gen=self.make_cone)
                glScalef(cone_scale, cone_scale, cone_scale)
                glColor3f(*self.color_config.get_color('cone'))
                glCallList(cone)
            else:
                if current_tool != self.cached_tool:
                    self.stale_dlist('tool')
                glCallList(self.dlist('tool', gen=lambda n: self.make_tool(n, current_tool)))

    def find_tool_placeholder_scale(self):
        if self.canon:
            g = self.canon
            x, y, z = 0, 1, 2
            return 0.5 * max(
                g.max_extents[x] - g.min_extents[x],
                g.max_extents[y] - g.min_extents[y],
                g.max_extents[z] - g.min_extents[z],
                2.)
        else:
            return 1

    def draw_foamwire_tool(self, pos, rx, ry):
        glEnable(GL_COLOR_MATERIAL)
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)
        glPushMatrix()
        glTranslatef(pos[0], pos[1], self.get_foam_z())
        glRotatef(180, 1, 0, 0)
        cone = self.dlist("cone", gen=self.make_cone)
        glColor3f(*self.color_config.get_color('cone_xy'))
        glCallList(cone)
        glPopMatrix()
        u = self.to_internal_linear_unit(rx)
        v = self.to_internal_linear_unit(ry)
        glPushMatrix()
        glTranslatef(u, v, self.get_foam_w())
        glColor3f(*self.color_config.get_color('cone_uv'))
        glCallList(cone)
        glPopMatrix()

    def draw_stat_overlay(self):
        """
        Draws the status overlay text on top of gremlin preview (things like position, dtg, offsets, etc.)
        :return:
        """
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        ypos = self.winfo_height()
        glOrtho(0.0, self.winfo_width(), 0.0, ypos, -1.0, 1.0)
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        limit, homed, posstrs, droposstrs = self.posstrs()
        charwidth, linespace, base = self.get_font_info()
        maxlen = max([len(p) for p in posstrs])
        pixel_width = charwidth * max(len(p) for p in posstrs)
        glDepthFunc(GL_ALWAYS)
        glDepthMask(GL_FALSE)
        glEnable(GL_BLEND)
        glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA)
        glColor3f(*self.color_config.get_color('overlay_background'))
        glBlendColor(0, 0, 0, 1 - self.color_config.get_alpha_or_default('overlay'))
        glBegin(GL_QUADS)
        glVertex3f(0, ypos, 1)
        glVertex3f(0, ypos - 8 - linespace * len(posstrs), 1)
        glVertex3f(pixel_width + 42, ypos - 8 - linespace * len(posstrs), 1)
        glVertex3f(pixel_width + 42, ypos, 1)
        glEnd()
        glDisable(GL_BLEND)
        ypos -= linespace + 5
        i = 0
        glColor3f(*self.color_config.get_color('overlay_foreground'))
        if not self.get_show_offsets():
            for string in posstrs:
                maxlen = max(maxlen, len(string))
                glRasterPos2i(5, ypos)
                for char in string:
                    glCallList(base + ord(char))
                # Tormach
                # Eliminate homed indicator as redundant and confusing
                # if i < len(homed) and homed[i]:
                #    glRasterPos2i(pixel_width + 8, ypos)
                #    glBitmap(13, 16, 0, 3, 17, 0, homeicon)
                if i < len(homed) and limit[i]:
                    glBitmap(13, 16, 0, 1, 17, 0, limiticon)
                ypos -= linespace
                i = i + 1
        if self.get_show_offsets():
            i = 0
            for string in droposstrs:
                maxlen = max(maxlen, len(string))
                glRasterPos2i(5, ypos)
                for char in string:
                    glCallList(base + ord(char))
                # Tormach
                # Eliminate homed indicator as redundant and confusing
                # if i < len(homed) and homed[i]:
                #    glRasterPos2i(charwidth *3, ypos)
                #    glBitmap(13, 16, 0, 3, 17, 0, homeicon)
                ypos -= linespace
                i = i + 1
        glDepthFunc(GL_LESS)
        glDepthMask(GL_TRUE)
        glPopMatrix()
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)

    def make_tool(self, tool_dlist, current_tool):
        glColor3f(*self.color_config.get_color('cone'))
        self.cached_tool = current_tool
        glNewList(tool_dlist, GL_COMPILE)
        if self.is_lathe() and current_tool and current_tool.orientation != 0:
            glBlendColor(0, 0, 0, self.color_config.get_alpha_or_default('lathetool'))
            self._draw_lathetool(current_tool)
        else:
            glBlendColor(0, 0, 0, self.color_config.get_alpha_or_default('tool'))
            if self.is_lathe():
                glRotatef(90, 0, 1, 0)
            else:
                dia = current_tool.diameter
                r = self.to_internal_linear_unit(dia) / 2.
                q = gluNewQuadric()
                glEnable(GL_LIGHTING)
                gluCylinder(q, r, r, 8*r, 32, 1)
                with gl_push_matrix():
                    glRotatef(180, 1, 0, 0)
                    gluDisk(q, 0, r, 32, 1)
                glTranslatef(0, 0, 8*r)
                gluDisk(q, 0, r, 32, 1)
                glDisable(GL_LIGHTING)
                gluDeleteQuadric(q)
        glEndList()

    def posstrs(self):
        s = self.stat
        limit = []
        for i,l in enumerate(s.limit):
            if s.axis_mask & (1<<i):
                limit.append(l)

        homed = []
        for i,h in enumerate(s.homed):
            if s.axis_mask & (1<<i):
                homed.append(h)

        if self.is_lathe() and not s.axis_mask & 2:
            homed.insert(1, 0)
            limit.insert(1, 0)

        if not self.get_joints_mode():
            if self.get_show_commanded():
                positions = s.position
            else:
                positions = s.actual_position

            if self.get_show_relative():
                positions = [(i-j) for i, j in zip(positions, s.tool_offset)]
                positions = [(i-j) for i, j in zip(positions, s.g5x_offset)]

                t = -s.rotation_xy
                t = math.radians(t)
                x = positions[0]
                y = positions[1]
                positions[0] = x * math.cos(t) - y * math.sin(t)
                positions[1] = x * math.sin(t) + y * math.cos(t)
                positions = [(i-j) for i, j in zip(positions, s.g92_offset)]

            if self.get_a_axis_wrapped():
                positions[3] = math.fmod(positions[3], 360.0)
                if positions[3] < 0: positions[3] += 360.0

            if self.get_b_axis_wrapped():
                positions[4] = math.fmod(positions[4], 360.0)
                if positions[4] < 0: positions[4] += 360.0

            if self.get_c_axis_wrapped():
                positions[5] = math.fmod(positions[5], 360.0)
                if positions[5] < 0: positions[5] += 360.0

            positions = self.to_internal_units(positions)
            axisdtg = self.to_internal_units(s.dtg)
            g5x_offset = self.to_internal_units(s.g5x_offset)
            g92_offset = self.to_internal_units(s.g92_offset)
            tlo_offset = self.to_internal_units(s.tool_offset)
            dtg = self.to_internal_linear_unit(s.distance_to_go)
            spd = self.to_internal_linear_unit(s.current_vel)

            if self.get_show_metric():
                positions = self.from_internal_units(positions, 1)
                axisdtg = self.from_internal_units(axisdtg, 1)
                g5x_offset = self.from_internal_units(g5x_offset, 1)
                g92_offset = self.from_internal_units(g92_offset, 1)
                tlo_offset = self.from_internal_units(tlo_offset, 1)
                dtg *= 25.4
                spd = spd * 25.4
            spd = spd * 60
            limit, homed, posstrs, droposstrs = self.dro_format(self.stat,spd,dtg,limit,homed,positions,axisdtg,g5x_offset,g92_offset,tlo_offset)
        else:
            # N.B. no conversion here because joint positions are unitless
            posstrs = ["  %s:% 9.4f" % i for i in
                zip(range(self.get_num_joints()), s.joint_actual_position)]
            droposstrs = posstrs
        return limit, homed, posstrs, droposstrs

    def dro_format(self,s,spd,dtg,limit,homed,positions,axisdtg,g5x_offset,g92_offset,tlo_offset):
        if self.get_show_metric():
            format = "% 6s:% 9.3f"
            droformat = " " + format + "  DTG %1s:% 9.3f"
            offsetformat = "% 5s %1s:% 9.3f  G92 %1s:% 9.3f"
            rotformat = "% 5s %1s:% 9.3f"
        else:
            format = "% 6s:% 9.4f"
            droformat = " " + format + "  DTG %1s:% 9.4f"
            offsetformat = "% 5s %1s:% 9.4f  G92 %1s:% 9.4f"
            rotformat = "% 5s %1s:% 9.4f"
        diaformat = " " + format

        posstrs = []
        droposstrs = []
        for i in range(9):
            a = "XYZABCUVW"[i]
            if s.axis_mask & (1<<i):
                posstrs.append(format % (a, positions[i]))
                droposstrs.append(droformat % (a, positions[i], a, axisdtg[i]))

        droposstrs.append("")

        for i in range(9):
            index = s.g5x_index
            if index<7:
                label = "G5%d" % (index+3)
            else:
                label = "G59.%d" % (index-6)

            a = "XYZABCUVW"[i]
            if s.axis_mask & (1<<i):
                droposstrs.append(offsetformat % (label, a, g5x_offset[i], a, g92_offset[i]))
        droposstrs.append(rotformat % (label, 'R', s.rotation_xy))

        droposstrs.append("")
        for i in range(9):
            a = "XYZABCUVW"[i]
            if s.axis_mask & (1<<i):
                droposstrs.append(rotformat % ("TLO", a, tlo_offset[i]))


        if self.is_lathe():
            posstrs[0] = format % ("Rad", positions[0])
            posstrs.insert(1, format % ("Dia", positions[0]*2.0))
            droposstrs[0] = droformat % ("Rad", positions[0], "R", axisdtg[0])
            droposstrs.insert(1, diaformat % ("Dia", positions[0]*2.0))

        if self.get_show_machine_speed():
            posstrs.append(format % ("Vel", spd))

        if self.get_show_distance_to_go():
            posstrs.append(format % ("DTG", dtg))

        return limit, homed, posstrs, droposstrs

    def draw_small_origin(self, n):
        glNewList(n, GL_COMPILE)
        r = 2.0/25.4
        glColor3f(*self.color_config.get_color('small_origin'))

        glBegin(GL_LINE_STRIP)
        for i in range(37):
            theta = (i*10)*math.pi/180.0
            glVertex3f(r*math.cos(theta),r*math.sin(theta),0.0)
        glEnd()
        glBegin(GL_LINE_STRIP)
        for i in range(37):
            theta = (i*10)*math.pi/180.0
            glVertex3f(0.0, r*math.cos(theta), r*math.sin(theta))
        glEnd()
        glBegin(GL_LINE_STRIP)
        for i in range(37):
            theta = (i*10)*math.pi/180.0
            glVertex3f(r*math.cos(theta),0.0, r*math.sin(theta))
        glEnd()

        glBegin(GL_LINES)
        glVertex3f(-r, -r, 0.0)
        glVertex3f( r,  r, 0.0)
        glVertex3f(-r,  r, 0.0)
        glVertex3f( r, -r, 0.0)

        glVertex3f(-r, 0.0, -r)
        glVertex3f( r, 0.0,  r)
        glVertex3f(-r, 0.0,  r)
        glVertex3f( r, 0.0, -r)

        glVertex3f(0.0, -r, -r)
        glVertex3f(0.0,  r,  r)
        glVertex3f(0.0, -r,  r)
        glVertex3f(0.0,  r, -r)
        glEnd()
        glEndList()

    def make_local_axes(self, dlist_name, letters="XYZ"):
        """
        Draws labeled axes at the current view origin.
        For example, to draw the work origin, work offset, xy rotation, and g92 offset all need to be "applied"
        :param dlist_name:
        :param letters:
        :return:
        """
        glNewList(dlist_name, GL_COMPILE)
        x,y,z,p = 0,1,2,3
        view = self.get_view()

        glColor3f(*self.color_config.get_color('axis_x'))
        glBegin(GL_LINES)
        if self.is_rear_tool_post_lathe():
            glVertex3f(-1.0,0.0,0.0)
        else:
            glVertex3f(1.0,0.0,0.0)
        glVertex3f(0.0,0.0,0.0)
        glEnd()

        if view != x:
            glPushMatrix()
            if self.is_lathe():
                if self.is_rear_tool_post_lathe():
                    glTranslatef(-1.3, 0, 0)
                    glRotatef(-90, 0, 0, 1)
                    glRotatef(90, 0, 1, 0)
                else:
                    glTranslatef(1.3, -0.1, 0)
                    glTranslatef(0, 0, -0.1)
                    glRotatef(-90, 0, 1, 0)
                    glRotatef(90, 1, 0, 0)
                    glTranslatef(0.1, 0, 0)
            else:
                glTranslatef(1.2, -0.1, 0)
                if view == y:
                    glTranslatef(0, 0, -0.1)
                    glRotatef(90, 1, 0, 0)
            glScalef(0.2, 0.2, 0.2)
            self.hershey.plot_string(letters[0], 0.5)
            glPopMatrix()

        glColor3f(*self.color_config.get_color('axis_y'))
        glBegin(GL_LINES)
        glVertex3f(0.0,0.0,0.0)
        glVertex3f(0.0,1.0,0.0)
        glEnd()

        if view != y:
            glPushMatrix()
            glTranslatef(0, 1.2, 0)
            if view == x:
                glTranslatef(0, 0, -0.1)
                glRotatef(90, 0, 1, 0)
                glRotatef(90, 0, 0, 1)
            glScalef(0.2, 0.2, 0.2)
            self.hershey.plot_string(letters[1], 0.5)
            glPopMatrix()

        glColor3f(*self.color_config.get_color('axis_z'))
        glBegin(GL_LINES)
        glVertex3f(0.0,0.0,0.0)
        glVertex3f(0.0,0.0,1.0)
        glEnd()

        if view != z:
            glPushMatrix()
            glTranslatef(0, 0, 1.2)
            if self.is_lathe():
                glRotatef(90, 0, 1, 0)
            if view == x:
                glRotatef(90, 0, 1, 0)
                glRotatef(90, 0, 0, 1)
            elif view == y or view == p:
                glRotatef(90, 1, 0, 0)
            if self.is_lathe():
                glTranslatef(0, -.1, 0)
            glScalef(0.2, 0.2, 0.2)
            self.hershey.plot_string(letters[2], 0.5)
            glPopMatrix()

        glEndList()

    def gl_draw_g92_label(self, g92_offset):
        x,y,z,p = 0,1,2,3
        s = self.stat
        view = self.get_view()

        glPushMatrix()

        glScalef(0.2, 0.2, 0.2)
        if self.is_lathe():
            g92rot = math.atan2(g92_offset[0], -g92_offset[2])
            glRotatef(90, 1, 0, 0)
            glRotatef(-90, 0, 0, 1)
        else:
            if view == x:
                g92rot = math.atan2(g92_offset[2], g92_offset[1])
                glRotatef(90, 1, 0, 0)
                glRotatef(90, 0, 1, 0)
            elif view == y:
                g92rot = math.atan2(g92_offset[2], g92_offset[0])
                glRotatef(90, 1, 0, 0)
            else:
                g92rot = math.atan2(g92_offset[1], g92_offset[0])

        glRotatef(math.degrees(g92rot), 0, 0, 1)
        glTranslatef(0.5, 0.5, 0)
        self.hershey.plot_string("G92", 0.1)
        glPopMatrix()

    def make_cone(self, n):
        q = gluNewQuadric()
        glNewList(n, GL_COMPILE)
        glEnable(GL_LIGHTING)
        gluCylinder(q, 0, .1, .25, 32, 1)
        glPushMatrix()
        glTranslatef(0, 0, .25)
        gluDisk(q, 0, .1, 32, 1)
        glPopMatrix()
        glDisable(GL_LIGHTING)
        glEndList()
        gluDeleteQuadric(q)

    def _draw_lathetool(self, current_tool):
        if self.is_rear_tool_post_lathe:
            lathe_shapes = [
                None,                           # 0
                (-1,-1), (-1,1), (1,1), (1,-1), # 1..4
                (0,-1), (-1,0), (0,1), (1,0),   # 5..8
                (0,0)                           # 9
            ]
        else:
            lathe_shapes = [
                None,                           # 0
                (1,-1), (1,1), (-1,1), (-1,-1), # 1..4
                (0,-1), (1,0), (0,1), (-1,0),   # 5..8
                (0,0)                           # 9
            ]

        glDepthFunc(GL_ALWAYS)
        diameter, frontangle, backangle, orientation = current_tool[-4:]
        w = 3/8.

        radius = self.to_internal_linear_unit(diameter) / 2.
        glColor3f(*self.color_config.get_color('lathetool'))
        glBegin(GL_LINES)  # Draw tool control point cross hairs
        glVertex3f(-radius/2.0,0.0,0.0)
        glVertex3f(radius/2.0,0.0,0.0)
        glVertex3f(0.0,0.0,-radius/2.0)
        glVertex3f(0.0,0.0,radius/2.0)
        glEnd()

        glNormal3f(0,1,0)

        if orientation == 9:
            glBegin(GL_TRIANGLE_FAN)
            for i in range(37):  # 10 degree increments in 360
                t = i * math.pi / 18  # angle in Radians
                glVertex3f(radius * math.cos(t), 0.0, radius * math.sin(t))
            glEnd()
        else:
#            dx, dy = self.lathe_shapes[orientation]
            dx, dy = lathe_shapes[orientation]
            min_angle = min(backangle, frontangle) * math.pi / 180
            max_angle = max(backangle, frontangle) * math.pi / 180

            sinmax = math.sin(max_angle)
            cosmax = math.cos(max_angle)
            tanmax = math.cos(max_angle)
            sinmin = math.sin(min_angle)
            cosmin = math.cos(min_angle)
            tanmin = math.cos(min_angle)

            circleminangle = - math.pi/2 + min_angle
            circlemaxangle = - 3*math.pi/2 + max_angle
            d0 = 0

            x1 = (w - d0)

            sz = max(w, 3*radius)

            glBegin(GL_TRIANGLE_FAN)
            glVertex3f(
                radius * dx + radius * math.sin(circleminangle) + sz * sinmin,
                0,
                radius * dy + radius * math.cos(circleminangle) + sz * cosmin)

            for i in range(37):
                #t = circleminangle + i * (circlemaxangle - circleminangle)/36.
                t = circleminangle + i * (circlemaxangle - circleminangle)/36.
                glVertex3f(radius*dx + radius * math.sin(t), 0.0, radius*dy + radius * math.cos(t))

            glVertex3f(
                radius * dx + radius * math.sin(circlemaxangle) + sz * sinmax,
                0,
                radius * dy + radius * math.cos(circlemaxangle) + sz * cosmax)

            glEnd()
        glDepthFunc(GL_LESS)

    def extents_info(self):
        if self.canon:
            mid = [(a+b) / 2 for a, b in zip(self.canon.max_extents, self.canon.min_extents)]
            sz_scale = .5 if self.is_lathe() else 1.0
            size = [sz_scale * (a-b) for a, b in zip(self.canon.max_extents, self.canon.min_extents)]
        else:  # use work space extents (limits) from .ini file
            s = self.stat
            sz_scale = 0.37 if self.is_lathe() else 0.43

            mid = [
                ((self.x_limit_positive + self.x_limit_negative) / 2) - s.tool_offset[0],
                ((self.y_limit_positive + self.y_limit_negative) / 2) - s.tool_offset[1],
                ((self.z_limit_positive + self.z_limit_negative) / 2) - s.tool_offset[2],
                ]

            size = [
                sz_scale * (self.x_limit_positive - self.x_limit_negative),
                sz_scale * (self.y_limit_positive - self.y_limit_negative),
                sz_scale * (self.z_limit_positive - self.z_limit_negative),
                ]

        return mid, size

    def make_nonrapids_list(self, nonrapids_list, for_selection=False):
        glNewList(nonrapids_list, GL_COMPILE)
        if self.canon:
            self.canon.draw_nonrapids(for_selection)
        glEndList()

    def make_rapids_list(self, rapids_list, for_selection=False):
        glNewList(rapids_list, GL_COMPILE)
        if self.canon:
            self.canon.draw_rapids(for_selection)
        glEndList()

    def make_highlight_list(self, highlight_list):
        line = self.get_highlight_line()
        glNewList(highlight_list, GL_COMPILE)
        if line is not None and self.canon is not None:
            if self.foam_option:
                glPushMatrix()
                glTranslatef(0, 0, self.get_foam_z())
                self.set_geometry("XY")
                self.canon.highlight(line)
                glTranslatef(0, 0, self.get_foam_w()-self.get_foam_z())
                self.set_geometry("UV")
                self.canon.highlight(line)
                glPopMatrix()
            else:
                self.canon.highlight(line)
        glEndList()

    def get_work_offsets_used(self):
        return self._work_offsets_used

    def load_preview(self, f, canon, unitcode, initcode, interpname=None, working_dir=None, start_line=0, leadin_type=None, keep_current_tool=True):
        self.set_canon(canon)
        print "preview working directory: ", working_dir
        print "preview start line: ", start_line
        print "preview leadin: ", leadin_type
        print "preview keep-current-tool: ", keep_current_tool
        result, seq, warnings = gcode.parse(
            file_name=f,
            canon_instance=canon,
            units_gcode=unitcode,
            init_gcode=initcode,
            interp_name=interpname or "",
            working_dir=working_dir or "",
            start_line_number=start_line,
            leadin_type=leadin_type,
            keep_current_tool=keep_current_tool,
        )

        # this is only valid right after a gcode.parse() as it will continue to change if the 
        # underlying interp is reset or reused for other files.  It gets cleared when a new file is
        # opened underneath.
        self._work_offsets_used = gcode.get_work_offsets_used()

        if result <= gcode.MIN_ERROR:
            if self.canon.progress is not None:
                self.canon.progress.nextphase(1)
            #canon.calc_extents()
            self.stale_dlist('program_rapids')
            self.stale_dlist('program_norapids')
            self.stale_dlist('select_rapids')
            self.stale_dlist('select_norapids')
            self.stale_dlist('highlight')

        return result, seq, warnings

    def from_internal_units(self, pos, unit=None):
        if unit is None:
            unit = self.stat.linear_units
        lu = (unit or 1) * 25.4 # units / inch

        lus = [lu, lu, lu, 1, 1, 1, lu, lu, lu]
        return [a*b for a, b in zip(pos, lus)]

    def get_highlight_line(self):
        return self.highlight_line

    def lkup_color(self, s):
        a = self.color_config.get_alpha_or_default(s)
        s = self.color_config.get_color(s)
        return [int(x * 255) for x in s + (a,)]

    def get_show_work_offset(self):
        return False

    def get_show_axis_offset(self):
        return True

    def is_lathe(self):
        return self.lathe_option

    def is_rear_tool_post_lathe(self):
        return self.rear_tp_lathe_option

    def get_show_commanded(self): return self.use_commanded
    def get_show_extents(self): return self.show_extents_option
    def get_show_limits(self): return self.show_limits
    def get_show_live_plot(self): return self.show_live_plot
    def get_show_machine_speed(self): return self.show_velocity
    def get_show_metric(self): return self.metric_units
    def get_show_program(self): return self.show_program
    def get_show_rapids(self): return self.show_rapids
    def get_show_relative(self): return self.use_relative
    def get_show_tool(self): return self.show_tool
    def get_show_distance_to_go(self): return self.show_dtg
    def get_show_stat_overlay(self): return self.show_stat_overlay
    def get_program_alpha(self): return self.program_alpha
    def get_num_joints(self): return self.num_joints
    def get_joints_mode(self): return self.use_joints_mode
    def get_grid_size(self): return self.grid_size

    def get_view(self):
        view_dict = {
            'x': 0,
            'y': 1,
            'z': 2,
            'p': 3
        }
        return view_dict.get(self.current_view, 3)

    def get_a_axis_wrapped(self): return self.a_axis_wrapped
    def get_b_axis_wrapped(self): return self.b_axis_wrapped
    def get_c_axis_wrapped(self): return self.c_axis_wrapped

    def get_current_tool(self):
        for i in self.stat.tool_table:
            if i[0] == self.stat.tool_in_spindle:
                return i

    def get_font_info(self):
        return self.font_charwidth, self.font_linespace, self.font_base

    def get_show_offsets(self): return self.show_offsets

    def get_logger_last(self):
        raise NotImplementedError

    def get_logger_world_transform(self):
        raise NotImplementedError

    def draw_live_plot_lines(self):
        raise NotImplementedError

# vim:ts=8:sts=4:sw=4:et:
