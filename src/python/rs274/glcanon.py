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

from rs274 import Translated, ArcsToSegmentsMixin
import math
from gl_context_managers import *
import linuxcnc
import gcode
from collections import defaultdict, namedtuple

MotionKey = namedtuple('MotionKey', ['motion_type', 'exec_mode', 'tool_no'])


class GLCanon(Translated, ArcsToSegmentsMixin):
    lineno = -1

    def __init__(self, color_config, geometry, inifile):
        """

        :type color_config: gremlin_draw.GlDrawColors
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

# vim:ts=8:sts=4:sw=4:et:
