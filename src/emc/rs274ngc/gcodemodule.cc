//    This is a component of AXIS, a front-end for emc
//    Copyright 2004, 2005, 2006 Jeff Epler <jepler@unpythonic.net> and
//    Chris Radek <chris@timeguy.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <Python.h>
#include <structmember.h>

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "interp_return.hh"
#include "canon.hh"
#include "config.h"		// LINELEN
#include <sstream>

int _task = 0; // control preview behaviour when remapping

extern "C" void initinterpreter();
extern "C" void initemccanon();
extern "C" struct _inittab builtin_modules[];
struct _inittab builtin_modules[] = {
    { const_cast<char*>("interpreter"), initinterpreter },
    { const_cast<char*>("emccanon"), initemccanon },
    // any others...
    { nullptr, nullptr }
};

static PyObject *int_array(int *arr, int sz) {
    PyObject *res = PyTuple_New(sz);
    for(int i = 0; i < sz; i++) {
        PyTuple_SET_ITEM(res, i, PyInt_FromLong(arr[i]));
    }
    return res;
}

struct LineCode {
    PyObject_HEAD
    double settings[ACTIVE_SETTINGS];
    int gcodes[ACTIVE_G_CODES];
    int mcodes[ACTIVE_M_CODES];
};

static PyObject *LineCode_gcodes(LineCode *l) {
    return int_array(l->gcodes, ACTIVE_G_CODES);
}
static PyObject *LineCode_mcodes(LineCode *l) {
    return int_array(l->mcodes, ACTIVE_M_CODES);
}

static PyGetSetDef LineCodeGetSet[] = {
    {const_cast<char*>("gcodes"), (getter)LineCode_gcodes, nullptr, nullptr, nullptr},
    {const_cast<char*>("mcodes"), (getter)LineCode_mcodes, nullptr, nullptr, nullptr},
    {},
};

static PyMemberDef LineCodeMembers[] = {
    {const_cast<char*>("sequence_number"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_SEQUENCE_NUMBER]), READONLY, nullptr},

    {const_cast<char*>("feed_rate"), T_DOUBLE,
     offsetof(LineCode, settings[SETTINGS_FEED_RATE]), READONLY, nullptr},
    {const_cast<char*>("speed"), T_DOUBLE,
     offsetof(LineCode, settings[SETTINGS_SPEED]), READONLY, nullptr},
    {const_cast<char*>("motion_mode"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_MOTION_MODE]), READONLY, nullptr},
    {const_cast<char*>("block"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_MODE_0]), READONLY, nullptr},
    {const_cast<char*>("plane"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_PLANE]), READONLY, nullptr},
    {const_cast<char*>("cutter_side"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_CUTTER_SIDE]), READONLY, nullptr},
    {const_cast<char*>("units"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_UNITS]), READONLY, nullptr},
    {const_cast<char*>("distance_mode"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_DISTANCE_MODE]), READONLY, nullptr},
    {const_cast<char*>("feed_mode"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_FEED_MODE]), READONLY, nullptr},
    {const_cast<char*>("origin"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_ORIGIN]), READONLY, nullptr},
    {const_cast<char*>("tool_length_offset"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_TOOL_LENGTH_OFFSET]), READONLY, nullptr},
    {const_cast<char*>("retract_mode"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_RETRACT_MODE]), READONLY, nullptr},
    {const_cast<char*>("path_mode"), T_INT,
     offsetof(LineCode, gcodes[G_CODE_PATH_MODE]), READONLY, nullptr},

    {const_cast<char*>("stopping"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_STOPPING]), READONLY, nullptr},
    {const_cast<char*>("spindle"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_SPINDLE]), READONLY, nullptr},
    {const_cast<char*>("toolchange"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_TOOLCHANGE]), READONLY, nullptr},
    {const_cast<char*>("mist"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_MIST]), READONLY, nullptr},
    {const_cast<char*>("flood"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_FLOOD]), READONLY, nullptr},
    {const_cast<char*>("overrides"), T_INT,
     offsetof(LineCode, mcodes[M_CODE_OVERRIDES]), READONLY, nullptr},
    {}
};

static PyTypeObject LineCodeType = {
    PyObject_HEAD_INIT(nullptr)
    0,                      /*ob_size*/
    "gcode.linecode",       /*tp_name*/
    sizeof(LineCode),       /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    nullptr,                      /*tp_dealloc*/
    nullptr,                      /*tp_print*/
    nullptr,                      /*tp_getattr*/
    nullptr,                      /*tp_setattr*/
    nullptr,                      /*tp_compare*/
    nullptr,                      /*tp_repr*/
    nullptr,                      /*tp_as_number*/
    nullptr,                      /*tp_as_sequence*/
    nullptr,                      /*tp_as_mapping*/
    nullptr,                      /*tp_hash*/
    nullptr,                      /*tp_call*/
    nullptr,                      /*tp_str*/
    nullptr,                      /*tp_getattro*/
    nullptr,                      /*tp_setattro*/
    nullptr,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    nullptr,                      /*tp_doc*/
    nullptr,                      /*tp_traverse*/
    nullptr,                      /*tp_clear*/
    nullptr,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    nullptr,                      /*tp_iter*/
    nullptr,                      /*tp_iternext*/
    nullptr,                      /*tp_methods*/
    LineCodeMembers,     /*tp_members*/
    LineCodeGetSet,      /*tp_getset*/
    nullptr,                      /*tp_base*/
    nullptr,                      /*tp_dict*/
    nullptr,                      /*tp_descr_get*/
    nullptr,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    nullptr,                      /*tp_init*/
    nullptr,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    nullptr,                      /*tp_free*/
    nullptr,                      /*tp_is_gc*/
};

static PyObject *callback;
static int interp_error;
static int last_sequence_number;
static bool metric;
static PositionArray _pos = {}; // In USER units
static PositionArray probe_start_pos = {};
static uint32_t max_glcanon_lines = 100000;
static uint32_t _linecount = 0;

// Track spindle synched motion
static int spindle_synched_motion = 0;
static double spindle_synched_feed_per_revolution;
static double spindle_speed_programmed = 0.0;
static double css_max_speed_programmed = 0.0;

static AxisOffsetParams cached_g92{}; // in USER units
static LocalToMachineTransform transform{}; // in USER units

enum class ParseExecMode {
    SEEK,
    PREP,
    RUN,
};

static ParseExecMode exec_mode_;

static InterpBase *pinterp;

#define callmethod(o, m, f, ...) PyObject_CallMethod((o), const_cast<char*>(m), const_cast<char*>(f), ## __VA_ARGS__)

#define SUPPRESS_NEXT_LINE_CB
// Experiment: suppress maybe new line since we don't need it for non-axis GUIs
// maybe_new_line doesn't seem to do anything if we don't actually have a
// callback connected, other than cost us overhead.
#ifndef SUPPRESS_NEXT_LINE_CB
static void maybe_new_line(int sequence_number=interp_new.sequence_number());
static void maybe_new_line(int sequence_number) {
    if(!pinterp) return;
    if(interp_error) return;
    if(sequence_number == last_sequence_number)
        return;
    LineCode *new_line_code =
        (LineCode*)(PyObject_New(LineCode, &LineCodeType));
    interp_new.active_settings(new_line_code->settings);
    interp_new.active_g_codes(new_line_code->gcodes);
    interp_new.active_m_codes(new_line_code->mcodes);
    new_line_code->gcodes[0] = sequence_number;
    last_sequence_number = sequence_number;
    PyObject *result =
        callmethod(callback, "next_line", "O", new_line_code);
    Py_DECREF(new_line_code);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
    _linecount++;
}
#else
static inline void maybe_new_line(int sequence_number=pinterp->sequence_number());
static inline void maybe_new_line(int sequence_number) {
    _linecount++;
    last_sequence_number = sequence_number;
}
#endif


void NURBS_FEED(int line_number, std::vector<CONTROL_POINT> nurbs_control_points, unsigned int k) {
    double u = 0.0;
    unsigned int n = nurbs_control_points.size() - 1;
    double umax = n - k + 2;
    unsigned int div = nurbs_control_points.size()*15;
    std::vector<unsigned int> knot_vector = knot_vector_creator(n, k);
    PLANE_POINT P1;
    while (u+umax/div < umax) {
        PLANE_POINT P1 = nurbs_point(u+umax/div,k,nurbs_control_points,knot_vector);
        STRAIGHT_FEED(line_number, P1.X,P1.Y, _pos[Z], _pos[A], _pos[B], _pos[C], _pos[U], _pos[V], _pos[W]);
        u = u + umax/div;
    }
    P1.X = nurbs_control_points[n].X;
    P1.Y = nurbs_control_points[n].Y;
    STRAIGHT_FEED(line_number, P1.X,P1.Y, _pos[Z], _pos[A], _pos[B], _pos[C], _pos[U], _pos[V], _pos[W]);
    knot_vector.clear();
}

void ARC_FEED(int line_number,
              double first_end, double second_end, double first_axis,
              double second_axis, int rotation, double axis_end_point,
              double a_position, double b_position, double c_position,
              double u_position, double v_position, double w_position) {
    // XXX: set _pos_*
    if(metric) {
        first_end /= 25.4;
        second_end /= 25.4;
        first_axis /= 25.4;
        second_axis /= 25.4;
        axis_end_point /= 25.4;
        u_position /= 25.4;
        v_position /= 25.4;
        w_position /= 25.4;
    }
    maybe_new_line(line_number);
    if(interp_error) return;
    if (_linecount < max_glcanon_lines) {
        PyObject *result =
            callmethod(callback, "arc_feed", "iffffifffffff",
                    line_number, first_end, second_end, first_axis, second_axis,
                    rotation, axis_end_point,
                    a_position, b_position, c_position,
                    u_position, v_position, w_position);
        if(result == nullptr) interp_error ++;
        Py_XDECREF(result);
    }
}

void STRAIGHT_FEED(int line_number,
                   double x, double y, double z,
                   double a, double b, double c,
                   double u, double v, double w) {
    _pos[X]=x; _pos[Y]=y; _pos[Z]=z;
    _pos[A]=a; _pos[B]=b; _pos[C]=c;
    _pos[U]=u; _pos[V]=v; _pos[W]=w;
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line(line_number);
    if(interp_error) return;
    if (_linecount < max_glcanon_lines) {
	PyObject *result = spindle_synched_motion ?
	    callmethod(callback, "straight_feed_synched", "ifffffffffff",
		       line_number, x, y, z, a, b, c, u, v, w,
               css_max_speed_programmed ?: spindle_speed_programmed,
		       spindle_synched_feed_per_revolution/(metric?25.4:1.0)) :
	    callmethod(callback, "straight_feed", "ifffffffff",
		       line_number, x, y, z, a, b, c, u, v, w);
    if(result == nullptr) interp_error ++;
	Py_XDECREF(result);
    }
}

void STRAIGHT_TRAVERSE(int line_number,
                       double x, double y, double z,
                       double a, double b, double c,
                       double u, double v, double w) {
    _pos[X]=x; _pos[Y]=y; _pos[Z]=z;
    _pos[A]=a; _pos[B]=b; _pos[C]=c;
    _pos[U]=u; _pos[V]=v; _pos[W]=w;
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line(line_number);
    if(interp_error) return;
    if (_linecount < max_glcanon_lines) {
        PyObject *result =
            callmethod(callback, "straight_traverse", "ifffffffff",
                    line_number, x, y, z, a, b, c, u, v, w);
        if(result == nullptr) interp_error ++;
        Py_XDECREF(result);
    }
}

void SET_G5X_TRANSFORM(int origin,
                       double x, double y, double z,
                       double a, double b, double c,
                       double u, double v, double w,
                       double xy_rotation_deg)
{
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line();
    if(interp_error) return;
    PyObject *result_offset =
        callmethod(callback, "set_g5x_transform", "iffffffffff",
                origin, x, y, z, a, b, c, u, v, w, xy_rotation_deg);
    if(!result_offset) interp_error ++;
    transform.g5xOffset = RotateShiftTransform({x, y, z, a, b, c, u, v, w}, xy_rotation_deg);
    Py_XDECREF(result_offset);
}

void SET_G92_OFFSET(bool applied, double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w) {
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line();
    if(interp_error) return;

    auto const &g92 = transform.g92Offset = ShiftTransform{{x,y,z,a,b,c,u,v,w}};
    cached_g92.applied = applied;
    if (applied) {
        cached_g92.offset = g92;
    }

    PyObject *result =
        callmethod(callback, "set_g92_offset", "fffffffff",
                   g92[X],
                   g92[Y],
                   g92[Z],
                   g92[A],
                   g92[B],
                   g92[C],
                   g92[U],
                   g92[V],
                   g92[W]);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void USE_LENGTH_UNITS(CANON_UNITS u) { metric = u == CANON_UNITS_MM; }

void SELECT_PLANE(CANON_PLANE pl) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "set_plane", "i", pl);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void SET_TRAVERSE_RATE(double rate) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "set_traverse_rate", "f", rate);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void SET_FEED_MODE(int mode) {
#if 0
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "set_feed_mode", "i", mode);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
#endif
}

void CHANGE_TOOL(int pocket) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "change_tool", "i", pocket);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

// FIXME does this have an effect on tool number / preview data organization?
void CHANGE_TOOL_NUMBER(int pocket) {
    maybe_new_line();
    if(interp_error) return;
}

/* XXX: This needs to be re-thought.  Sometimes feed rate is not in linear
 * units--e.g., it could be inverse time feed mode.  in that case, it's wrong
 * to convert from mm to inch here.  but the gcode time estimate gets inverse
 * time feed wrong anyway..
 */
void SET_FEED_RATE(double rate) {
    maybe_new_line();
    if(interp_error) return;
    if(metric) rate /= 25.4;
    PyObject *result =
        callmethod(callback, "set_feed_rate", "f", rate);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void DWELL(double time) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "dwell", "f", time);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void MESSAGE(char *comment) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "message", "s", comment);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void LOG(char *s) {}
void LOGOPEN(char *f) {}
void LOGAPPEND(char *f) {}
void LOGCLOSE() {}

void COMMENT(const char *comment) {
    maybe_new_line();
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "comment", "s", comment);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void SET_TOOL_TABLE_ENTRY(int pocket, int toolno, EmcPose offset, double diameter,
                          double frontangle, double backangle, int orientation) {
}

void USE_TOOL_LENGTH_OFFSET(EmcPose offset) {
    transform.toolOffset = ShiftTransform(toArray(offset));
    maybe_new_line();
    if(interp_error) return;
    if(metric) {
        offset.tran.x /= 25.4; offset.tran.y /= 25.4; offset.tran.z /= 25.4;
        offset.u /= 25.4; offset.v /= 25.4; offset.w /= 25.4; }
    PyObject *result = callmethod(callback, "tool_offset", "ddddddddd", offset.tran.x, offset.tran.y, offset.tran.z,
        offset.a, offset.b, offset.c, offset.u, offset.v, offset.w);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}

void SET_FEED_REFERENCE(double reference) { }
void SET_CUTTER_RADIUS_COMPENSATION(double radius) {}
void START_CUTTER_RADIUS_COMPENSATION(int direction) {}
void STOP_CUTTER_RADIUS_COMPENSATION(int direction) {}
void START_SPEED_FEED_SYNCH() {
    spindle_synched_motion = 1;
}
void START_SPEED_FEED_SYNCH(double sync, bool vel) {
    spindle_synched_motion = 1;
    spindle_synched_feed_per_revolution = sync;
}
void STOP_SPEED_FEED_SYNCH() {
    spindle_synched_motion = 0;
}
void START_SPINDLE_COUNTERCLOCKWISE() {}
void START_SPINDLE_CLOCKWISE() {}
void SET_SPINDLE_MODE(double css_max) {
    css_max_speed_programmed = css_max;
}

void STOP_SPINDLE_TURNING() {}
void SET_SPINDLE_SPEED(double rpm) {
    spindle_speed_programmed = rpm;
}
void ORIENT_SPINDLE(double d, int i) {}
void WAIT_SPINDLE_ORIENT_COMPLETE(double timeout) {}
void PROGRAM_STOP() {}
void UPDATE_PART_COUNT(size_t, int) {}
PartCountArray GET_EXTERNAL_PART_COUNTS() {return PartCountArray{};}
void PROGRAM_END(bool reset) {}
void FINISH() {}
void ON_RESET() {}
void PALLET_SHUTTLE() {}
void SELECT_POCKET(int pocket, int tool) {}
void UPDATE_TAG(StateTag &tag) {}
void SEND_TAG_MSG() {}
void OPTIONAL_PROGRAM_STOP() {}
void START_CHANGE() {}
int  GET_EXTERNAL_TC_FAULT() {return 0;}
int  GET_EXTERNAL_TC_REASON() {return 0;}


extern bool GET_BLOCK_DELETE(void) {
    int bd = 0;
    if(interp_error) return 0;
    PyObject *result =
        callmethod(callback, "get_block_delete", "");
    if(result == nullptr) {
        interp_error++;
    } else {
        bd = PyObject_IsTrue(result);
    }
    Py_XDECREF(result);
    return bd;
}

void CANON_ERROR(const char *fmt, ...) {};
void CLAMP_AXIS(CANON_AXIS axis) {}
bool GET_OPTIONAL_PROGRAM_STOP() { return false;}
void SET_OPTIONAL_PROGRAM_STOP(bool state) {}
void SPINDLE_RETRACT_TRAVERSE() {}
void SPINDLE_RETRACT() {}
void STOP_CUTTER_RADIUS_COMPENSATION() {}
void USE_NO_SPINDLE_FORCE() {}
void SET_BLOCK_DELETE(bool enabled) {}

void DISABLE_FEED_OVERRIDE() {}
void DISABLE_FEED_HOLD() {}
void ENABLE_FEED_HOLD() {}
void DISABLE_SPEED_OVERRIDE() {}
void ENABLE_FEED_OVERRIDE() {}
void ENABLE_SPEED_OVERRIDE() {}
void MIST_OFF() {}
void FLOOD_OFF() {}
void MIST_ON() {}
void FLOOD_ON() {}
void CLEAR_AUX_OUTPUT_BIT(int bit) {}
void SET_AUX_OUTPUT_BIT(int bit) {}
void SET_AUX_OUTPUT_VALUE(int index, double value) {}
void CLEAR_MOTION_OUTPUT_BIT(int bit) {}
void SET_MOTION_OUTPUT_BIT(int bit) {}
void SET_MOTION_OUTPUT_VALUE(int index, double value) {}
void TURN_PROBE_ON() {}
void TURN_PROBE_OFF() {}
int UNLOCK_ROTARY(int line_no, int axis) {return 0;}
int LOCK_ROTARY(int line_no, int axis) {return 0;}
void INTERP_ABORT(int reason,const char *message) {}
void PLUGIN_CALL(int len, const char *call) {}
void IO_PLUGIN_CALL(int len, const char *call) {}

void STRAIGHT_PROBE(int line_number,
                    double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w, unsigned char probe_type) {
    probe_start_pos = _pos;
    _pos[X]=x; _pos[Y]=y; _pos[Z]=z;
    _pos[A]=a; _pos[B]=b; _pos[C]=c;
    _pos[U]=u; _pos[V]=v; _pos[W]=w;

    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line(line_number);
    if(interp_error) return;
    if (_linecount < max_glcanon_lines) {
        PyObject *result =
            callmethod(callback, "straight_probe", "ifffffffff",
                    line_number, x, y, z, a, b, c, u, v, w);
        if(result == nullptr) interp_error ++;
        Py_XDECREF(result);
    }
}

void STRAIGHT_PROBE_TO_LIMIT(int line_number,
                    double x, double y, double z,
                    double a, double b, double c,
                    double u, double v, double w, unsigned char probe_type) {
    _pos[X]=x; _pos[Y]=y; _pos[Z]=z;
    _pos[A]=a; _pos[B]=b; _pos[C]=c;
    _pos[U]=u; _pos[V]=v; _pos[W]=w;
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; u /= 25.4; v /= 25.4; w /= 25.4; }
    maybe_new_line(line_number);
    if(interp_error) return;
    if (_linecount < max_glcanon_lines) {
        PyObject *result =
            callmethod(callback, "straight_probe_to_limit", "ifffffffff",
                    line_number, x, y, z, a, b, c, u, v, w);
        if(result == nullptr) interp_error ++;
        Py_XDECREF(result);
    }
}

void RIGID_TAP(int line_number,
    double x,
    double y,
    double z,
    double spindle_retract_speed_scale,
    double tap_pitch,
    double retract_pitch
){
    if(metric) { x /= 25.4; y /= 25.4; z /= 25.4; }
    maybe_new_line(line_number);
    if(interp_error) return;
    PyObject *result =
        callmethod(callback, "rigid_tap", "ifff",
            line_number, x, y, z);
    if(result == nullptr) interp_error ++;
    Py_XDECREF(result);
}
double GET_EXTERNAL_MOTION_CONTROL_TOLERANCE() { return 0.1; }
PositionArray GET_EXTERNAL_PROBE_POSITION()
{
    // KLUDGE if we're about to start a prep move, and are getting the external probe position (to restore the actual position), cheat and pretend the probe stopped right at the beginning
    if (exec_mode_ == ParseExecMode::PREP) {
        _pos = probe_start_pos;
    }
    return _pos;
}
double GET_EXTERNAL_PROBE_POSITION_X() { return _pos[X]; }
double GET_EXTERNAL_PROBE_POSITION_Y() { return _pos[Y]; }
double GET_EXTERNAL_PROBE_POSITION_Z() { return _pos[Z]; }
double GET_EXTERNAL_PROBE_POSITION_A() { return _pos[A]; }
double GET_EXTERNAL_PROBE_POSITION_B() { return _pos[B]; }
double GET_EXTERNAL_PROBE_POSITION_C() { return _pos[C]; }
double GET_EXTERNAL_PROBE_POSITION_U() { return _pos[U]; }
double GET_EXTERNAL_PROBE_POSITION_V() { return _pos[V]; }
double GET_EXTERNAL_PROBE_POSITION_W() { return _pos[W]; }
double GET_EXTERNAL_PROBE_VALUE() { return 0.0; }
int GET_EXTERNAL_PROBE_TRIPPED_VALUE() { return 0; }
PositionArray GET_EXTERNAL_POSITION() { return _pos; }
double GET_EXTERNAL_POSITION_X() { return _pos[X]; }
double GET_EXTERNAL_POSITION_Y() { return _pos[Y]; }
double GET_EXTERNAL_POSITION_Z() { return _pos[Z]; }
double GET_EXTERNAL_POSITION_A() { return _pos[A]; }
double GET_EXTERNAL_POSITION_B() { return _pos[B]; }
double GET_EXTERNAL_POSITION_C() { return _pos[C]; }
double GET_EXTERNAL_POSITION_U() { return _pos[U]; }
double GET_EXTERNAL_POSITION_V() { return _pos[V]; }
double GET_EXTERNAL_POSITION_W() { return _pos[W]; }
void INIT_CANON() {}

void SET_PARAMETER_FILE_NAME(const char *name)
{
    PyObject *fname = PyString_FromString(name);
    PyObject_SetAttrString(callback, "parameter_file", fname);
    Py_XDECREF(fname);
}

void GET_EXTERNAL_PARAMETER_FILE_NAME(char *name, int max_size) {
    PyObject *result = PyObject_GetAttrString(callback, "parameter_file");
    if(!result) { name[0] = 0; return; }
    char *s = PyString_AsString(result);
    if(!s) { name[0] = 0; return; }
    memset(name, 0, max_size);
    strncpy(name, s, max_size - 1);
}

CANON_UNITS GET_EXTERNAL_LENGTH_UNIT_TYPE() { return metric ? CANON_UNITS_MM : CANON_UNITS_INCHES; }

CANON_TOOL_TABLE GET_EXTERNAL_TOOL_TABLE(int pocket) {
    CANON_TOOL_TABLE t = {-1,{{0,0,0},0,0,0,0,0,0},0,0,0,0};
    if(interp_error) return t;
    PyObject *result =
        callmethod(callback, "get_tool", "i", pocket);
    if(result == nullptr ||
       !PyArg_ParseTuple(result, "iddddddddddddi", &t.toolno, &t.offset.tran.x, &t.offset.tran.y, &t.offset.tran.z,
                          &t.offset.a, &t.offset.b, &t.offset.c, &t.offset.u, &t.offset.v, &t.offset.w,
                          &t.diameter, &t.frontangle, &t.backangle, &t.orientation))
            interp_error ++;

    Py_XDECREF(result);
    return t;
}

RotateShiftTransform GET_EXTERNAL_WORK_OFFSET(int origin) {
    RotateShiftTransform t = {};
    double xy_rotation_deg = 0.0;
    if(interp_error) return t;
    PyObject *result =
        callmethod(callback, "get_g5x_transform", "i", origin);
    if(result == nullptr ||
       !PyArg_ParseTuple(result, "dddddddddd", UNPACK_POSITION_ARRAY(t.shift.tf), &xy_rotation_deg))
            interp_error ++;

    Py_XDECREF(result);
    t.setXYRotationDeg(xy_rotation_deg);
    return t;
}

AxisOffsetParams GET_EXTERNAL_PERSISTENT_AXIS_OFFSET() {
    return cached_g92;
}

int GET_EXTERNAL_DIGITAL_INPUT(int index, int def) { return def; }
int GET_EXTERNAL_ORIENT_TIMEOUT() { return 0; }
double GET_EXTERNAL_ANALOG_INPUT(int index, double def) { return def; }
int WAIT(int index, CANON_INPUT input_type, CANON_WAIT_MODE wait_type, double timeout) { return 0;}

static void user_defined_function(int num, double arg1, double arg2) {
    if(interp_error) return;
    maybe_new_line();
    PyObject *result =
        callmethod(callback, "user_defined_function",
                            "idd", num, arg1, arg2);
    if(result == nullptr) interp_error++;
    Py_XDECREF(result);
}

void SET_FEED_REFERENCE(CANON_FEED_REFERENCE ref) {}
int GET_EXTERNAL_QUEUE_EMPTY() { return true; }
CANON_DIRECTION GET_EXTERNAL_SPINDLE() { return CANON_STOPPED; }
int GET_EXTERNAL_TOOL_SLOT() { return 0; }
int GET_EXTERNAL_SELECTED_TOOL_SLOT() { return 0; }
// TODO support preview-level start line seek to show expected execution
bool IN_START_LINE_SEEK() {
    switch(exec_mode_) {
    case ParseExecMode::SEEK:
        return true;
    case ParseExecMode::PREP:
    case ParseExecMode::RUN:
        return false;
    }
    return false;
}
double GET_EXTERNAL_FEED_RATE() { return 1; }
double GET_EXTERNAL_TRAVERSE_RATE() { return 0; }
int GET_EXTERNAL_FLOOD() { return 0; }
int GET_EXTERNAL_MIST() { return 0; }
CANON_PLANE GET_EXTERNAL_PLANE() { return CANON_PLANE_XY; }
double GET_EXTERNAL_SPEED() { return 0; }
int GET_EXTERNAL_POCKETS_MAX() { return CANON_POCKETS_MAX; }
void DISABLE_ADAPTIVE_FEED() {}
void ENABLE_ADAPTIVE_FEED() {}

int GET_EXTERNAL_FEED_OVERRIDE_ENABLE() {return 1;}
int GET_EXTERNAL_SPINDLE_OVERRIDE_ENABLE() {return 1;}
int GET_EXTERNAL_ADAPTIVE_FEED_ENABLE() {return 0;}
int GET_EXTERNAL_FEED_HOLD_ENABLE() {return 1;}

int GET_EXTERNAL_AXIS_MASK() {
    if(interp_error) return 7;
    PyObject *result =
        callmethod(callback, "get_axis_mask", "");
    if(!result) { interp_error ++; return 7 /* XYZABC */; }
    if(!PyInt_Check(result)) { interp_error ++; return 7 /* XYZABC */; }
    int mask = PyInt_AsLong(result);
    Py_DECREF(result);
    return mask;
}

double GET_EXTERNAL_TOOL_LENGTH_XOFFSET() {
    return transform.toolOffset[X];
}
double GET_EXTERNAL_TOOL_LENGTH_YOFFSET() {
    return transform.toolOffset[Y];
}
double GET_EXTERNAL_TOOL_LENGTH_ZOFFSET() {
    return transform.toolOffset[Z];
}
double GET_EXTERNAL_TOOL_LENGTH_AOFFSET() {
    return transform.toolOffset[A];
}
double GET_EXTERNAL_TOOL_LENGTH_BOFFSET() {
    return transform.toolOffset[B];
}
double GET_EXTERNAL_TOOL_LENGTH_COFFSET() {
    return transform.toolOffset[C];
}
double GET_EXTERNAL_TOOL_LENGTH_UOFFSET() {
    return transform.toolOffset[U];
}
double GET_EXTERNAL_TOOL_LENGTH_VOFFSET() {
    return transform.toolOffset[V];
}
double GET_EXTERNAL_TOOL_LENGTH_WOFFSET() {
    return transform.toolOffset[W];
}

static bool PyInt_CheckAndError(const char *func, PyObject *p)  {
    if(PyInt_Check(p)) return true;
    PyErr_Format(PyExc_TypeError,
            "%s: Expected int, got %s", func, p->ob_type->tp_name);
    return false;
}

static bool PyFloat_CheckAndError(const char *func, PyObject *p)  {
    if(PyFloat_Check(p)) return true;
    PyErr_Format(PyExc_TypeError,
            "%s: Expected float, got %s", func, p->ob_type->tp_name);
    return false;
}

double GET_EXTERNAL_ANGLE_UNITS() {
    PyObject *result =
        callmethod(callback, "get_external_angular_units", "");
    if(result == nullptr) interp_error++;

    double dresult = 1.0;
    if(!result || !PyFloat_CheckAndError("get_external_angle_units", result)) {
        interp_error++;
    } else {
        dresult = PyFloat_AsDouble(result);
    }
    Py_XDECREF(result);
    return dresult;
}

double GET_EXTERNAL_LENGTH_UNITS() {
    PyObject *result =
        callmethod(callback, "get_external_length_units", "");
    if(result == nullptr) interp_error++;

    double dresult = 0.03937007874016;
    if(!result || !PyFloat_CheckAndError("get_external_length_units", result)) {
        interp_error++;
    } else {
        dresult = PyFloat_AsDouble(result);
    }
    Py_XDECREF(result);
    return dresult;
}

static bool check_abort() {
    PyObject *result =
        callmethod(callback, "check_abort", "");
    if(!result) return 1;
    if(PyObject_IsTrue(result)) {
        Py_DECREF(result);
        PyErr_Format(PyExc_KeyboardInterrupt, "Load aborted");
        return 1;
    }
    Py_DECREF(result);
    return 0;
}

USER_DEFINED_FUNCTION_TYPE USER_DEFINED_FUNCTION[USER_DEFINED_FUNCTION_NUM];

static CANON_MOTION_MODE motion_mode;
void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode, double tolerance) { motion_mode = mode; }
void SET_MOTION_CONTROL_MODE(double tolerance) { }
void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode) { motion_mode = mode; }
CANON_MOTION_MODE GET_EXTERNAL_MOTION_CONTROL_MODE() { return motion_mode; }
void SET_NAIVECAM_TOLERANCE(double tolerance) { }

static PyObject *get_work_offsets_used(PyObject *self, PyObject *args) {

    // call pinterp to snag the set of work offset indexes and build a new list to 
    // give back to python land
    std::set<int> used_offsets = reinterpret_cast<Interp*>(pinterp)->get_work_offsets_used();

    PyObject* offsets = PyTuple_New(used_offsets.size());
    int ix = 0;
    for (auto offset : used_offsets)
    {
       PyTuple_SetItem(offsets, ix, PyInt_FromLong(offset));
       ++ix;
    }   
    return offsets;
}

// KLUDGE provide a place to attach a debugger for the RETURN_ON_INTERP_ERROR macro.
// The err_count value is meaningless, except that a change means somebody threw an error.
static unsigned err_dbg_count=0;
static void count_err()
{
    err_dbg_count++;
}

#define RETURN_ON_INTERP_ERROR(interp_call) do { \
    int result = interp_call; \
    switch(result) { \
    case INTERP_OK: \
    case INTERP_EXECUTE_FINISH: \
        break; \
    default: \
        count_err(); \
        return result; \
    } \
    } while (0) \

int execute_lines(InterpBase *interp, const char *multiline_cmd, CommandSource cmd_src)
{
    std::stringstream cmd(multiline_cmd);
    std::string s;
    while (std::getline(cmd, s, '\n')) {
        RETURN_ON_INTERP_ERROR(interp->execute(s.c_str(), cmd_src));
    }
    return INTERP_OK;
}

static PyObject * gen_warning_list(std::vector<std::string> const &interp_warning_list)
{
    PyObject *warning_list = PyList_New(interp_warning_list.size());
    for (size_t i=0; i < interp_warning_list.size(); ++i) {
        PyList_SetItem(warning_list, i, PyString_FromString(interp_warning_list[i].c_str()));
    }
    return warning_list;
}

// Crude version of CHP for the parse loop
#define RETURN_ON_ERROR(call) \
do { \
    int err_status = (call); \
    if (isInterpError(err_status)) { \
        return err_status; \
    } \
} while(0)

int set_exec_mode(ParseExecMode mode)
{
    exec_mode_ = mode;
    if(interp_error) return INTERP_ERROR;
    PyObject *result =
        callmethod(callback, "set_exec_mode", "i", (int)mode);
    if(result == nullptr) {
        interp_error ++;
        return INTERP_ERROR;
    }
    Py_XDECREF(result);
    return INTERP_OK;
}

int synch_position(PositionArray const &machine_pos)
{
    if(interp_error) return INTERP_ERROR;
    PyObject *result =
        callmethod(callback, "synch_position", "fffffffff",
                   UNPACK_POSITION_ARRAY_REF(machine_pos));
    if(result == nullptr) {
        interp_error ++;
        return INTERP_ERROR;
    }
    Py_XDECREF(result);
    return INTERP_OK;
}

void restore_machine_state_before_preview_synch(ExpandedModalState const &run_initial_state)
{
    // KLUDGE force the "machine" state to be what the initial position was again, as if synching from a real machine
    _pos = transform.inverse_local_from_nose(run_initial_state.initial_nose_position_uu);

    // TODO handle coolant settings here if we ever show coolant state in preview
    // flood
    // mist
}

int planFinishLineSeek(
    Interp *rs274_interp,
    AutoRunSettings const &run_settings,
    ExpandedModalState const &run_initial_state
){
    ExpandedModalState state_at_seek{};
    int res_interp = INTERP_OK;
    if (rs274_interp) {
        // NOTE: modal state is recorded before the transition for use in the restore sequence
        res_interp = rs274_interp->cbBeforeSeekReadTransition(run_settings, run_initial_state, state_at_seek);
    }

    maybe_new_line();
    RETURN_ON_ERROR(pinterp->execute("", CommandSource::MODAL_STATE_RESTORE));
    set_exec_mode(ParseExecMode::PREP);
    restore_machine_state_before_preview_synch(run_initial_state);

    if(interp_error) return INTERP_ERROR;
    auto machine_pos = transform.forward_local_to_tip(_pos);
    RETURN_ON_ERROR(synch_position(machine_pos));

    if (!isInterpError((InterpReturn)res_interp) && rs274_interp) {
        // Do a full modal state restore as if the program had executed from line 1,
        // and insert any preparatory actions like tool changes / lead-in moves.
        // FIXME synch may return garbage since the globals that are equivalent to machine state are directly poked in canon
        res_interp = rs274_interp->cbAfterSeekReadTransition(state_at_seek, run_settings, run_initial_state);
    } else {
        // Legacy behavior that just synchs up with world without doing explicit modal state restore
        rs274_interp->synch();
    }

    return res_interp;
}

static bool isStartLineNext(int start_line) {
    return (pinterp->line() + 1 >= start_line && pinterp->call_level() == 0);
}

static int do_parse_loop(
    Interp *rs274_interp,
    char *file_name,
    char *units_gcode,
    char *init_gcode,
    char *working_dir,
    int &error_line_offset,
    AutoRunSettings start_settings,
    int wait
){
    struct timeval t0{}, t1{};
    gettimeofday(&t0, nullptr);

    RETURN_ON_ERROR(pinterp->init());

    if (rs274_interp) {
        //
        auto UtoP = rs274_interp->convertsUserToProgram();
        auto PtoU = rs274_interp->convertsProgramToUser();
        transform.g5xOffset = PtoU.forward_tf(rs274_interp->_setup.transform.g5xOffset);
        transform.g92Offset = PtoU.forward_tf(rs274_interp->_setup.transform.g92Offset);
        transform.toolOffset = PtoU.forward_tf(rs274_interp->_setup.transform.g92Offset);
        // KLUDGE pull the super secret current position out of the reserved parameters and update both interp and our cached position
        _pos = toArray(
            rs274_interp->_setup.parameters.begin() + interp_param_global::EXPORT_CUR_POS_ARRAY,
            rs274_interp->_setup.parameters.begin() + interp_param_global::EXPORT_CUR_POS_ARRAY_END);
        rs274_interp->_setup.current = UtoP.forward(_pos);
        auto machine_pos_user = transform.forward_local_to_tip(_pos);
        RETURN_ON_ERROR(synch_position(machine_pos_user));
    }

    // Copy current position back out of reserved parameter area

    RETURN_ON_ERROR(pinterp->open(file_name, working_dir));

    maybe_new_line();

    if(units_gcode) {
        RETURN_ON_ERROR(execute_lines(pinterp, units_gcode, CommandSource::INIT));
    }
    if(init_gcode) {
        RETURN_ON_ERROR(execute_lines(pinterp, init_gcode, CommandSource::INIT));
    }

    ExpandedModalState run_initial_state;
    pinterp->write_expanded_modal_state(run_initial_state);
    set_exec_mode(ParseExecMode::SEEK);

    while(!interp_error) {
        gettimeofday(&t1, nullptr);
        if(t1.tv_sec > t0.tv_sec + wait) {
            if(check_abort()) return 255;
            t0 = t1;
        }

        switch (exec_mode_) {
        case ParseExecMode::SEEK:
            // NOTE: current line has been read here but not executed
            if (isStartLineNext(start_settings.start_line)) {
                RETURN_ON_ERROR(planFinishLineSeek(rs274_interp, start_settings, run_initial_state));
            }
            break;
        case ParseExecMode::PREP:
            // KLUDGE Start run mode after G80.9 (we know this is the last step in the restore, but it's a subtle side effect and I don't like it)
            if (rs274_interp && rs274_interp->_setup.start_line_motion_mode == G_INVALID) {
                set_exec_mode(ParseExecMode::RUN);
            }
            break;
        case ParseExecMode::RUN:
            // Do nothing, already in run mode
            break;
        }
        error_line_offset = 1;
        RETURN_ON_ERROR(pinterp->read());
        error_line_offset = 0;
        InterpReturn result = (InterpReturn)pinterp->execute(nullptr, CommandSource::PROGRAM);
        switch(result)
        {
        case INTERP_OK:
        case INTERP_EXECUTE_FINISH:
            break;
        default:
            return result;
        }
    }
    return 0;
}

void reset_gcode_canon_globals(const char *interp_name)
{
    if(pinterp) {
        delete pinterp;
        pinterp = nullptr;
    }
    if(interp_name && *interp_name)
        pinterp = interp_from_shlib(interp_name);
    if(!pinterp)
        pinterp = new Interp;

    for(int i=0; i<USER_DEFINED_FUNCTION_NUM; i++)
        USER_DEFINED_FUNCTION[i] = user_defined_function;

    metric=false;
    interp_error = 0;
    last_sequence_number = -1;
    spindle_synched_motion = 0;
    spindle_speed_programmed = 0.0;

    _pos.fill(0);
    _linecount=0;
}

static PyObject *parse_file(PyObject *self, PyObject *args, PyObject *kwargs) {
    char *file_name = nullptr;
    char *units_gcode = nullptr;
    char *init_gcode = nullptr;
    char *interp_name = nullptr;
    char *working_dir = nullptr;
    int error_line_offset = 0;
    AutoRunSettings run_settings;
    int wait = 1;

    static const char * const kwarg_list[] = {
        "file_name",
        "canon_instance",
        "units_gcode",
        "init_gcode",
        "interp_name",
        "working_dir",
        "start_line_number",
        "leadin_type",
        "keep_current_tool",
        nullptr,
    };

    // KLUDGE shove even more parameters in here...
    if(!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "sO|ssssiii",
            const_cast<char**>(kwarg_list),
            &file_name,
            &callback,
            &units_gcode,
            &init_gcode,
            &interp_name,
            &working_dir,
            &run_settings.start_line,
            &run_settings.leadin_type,
            &run_settings.keep_current_tool))
        return nullptr;

    // reset canon global variables and flags before parsing
    reset_gcode_canon_globals(interp_name);

    Interp* interp = dynamic_cast<Interp*>(pinterp);
    int result = do_parse_loop(interp, file_name, units_gcode, init_gcode, working_dir, error_line_offset, run_settings, wait);

    if(pinterp) pinterp->close();
    if(interp_error) {
        if(!PyErr_Occurred()) {
            PyErr_Format(PyExc_RuntimeError,
                    "interp_error > 0 but no Python exception set");
        }
        return nullptr;
    }
    PyErr_Clear();
    maybe_new_line();
    if(PyErr_Occurred()) {
        interp_error = 1;
        return nullptr;
    }
    PyObject *retval = PyTuple_New(3);
    PyTuple_SET_ITEM(retval, 0, PyInt_FromLong(result));
    PyTuple_SET_ITEM(retval, 1, PyInt_FromLong(last_sequence_number + error_line_offset));

    if (interp) {
        PyObject *warnings = gen_warning_list(interp->getWarnings());

        // Throw a warning if we're clipping lines
        if (_linecount > max_glcanon_lines) {
            std::stringstream preview_cutoff_warning;
            preview_cutoff_warning << "Program exceeds " << max_glcanon_lines << " motion lines (" << _linecount << " total), displayed extents may not be accurate.  Use the context menu of the g-code window on the Main tab to load all preview lines.  Or use ADMIN SET_PREVIEW_LIMIT MDI command to change number of lines to interpret on program load.";
            PyObject *msg = PyString_FromString(preview_cutoff_warning.str().c_str());
            PyList_Append(warnings, msg);
            Py_DecRef(msg);
        }
        PyTuple_SET_ITEM(retval, 2, warnings);
    } else {
        PyTuple_SET_ITEM(retval, 2, PyList_New(0));
    }

    return retval;
}

static int maxerror = -1;

static PyObject *rs274_strerror(PyObject *s, PyObject *o) {
    int err;
    if(!PyArg_ParseTuple(o, "i", &err)) return nullptr;
    char error_buf[ERROR_TEXT_LEN]={};
    pinterp->error_text(err, error_buf, ERROR_TEXT_LEN);
    return PyString_FromString(error_buf);
}

static PyObject *rs274_calc_extents_dict(PyObject *self, PyObject *args) {
    double min_x = 9e99, min_y = 9e99, min_z = 9e99,
           min_xt = 9e99, min_yt = 9e99, min_zt = 9e99,
           max_x = -9e99, max_y = -9e99, max_z = -9e99,
           max_xt = -9e99, max_yt = -9e99, max_zt = -9e99;

    PyObject *lines_dict = nullptr;
    int r = PyArg_ParseTuple(
        args,
        "O!:calc_extents dict lookup",
        &PyDict_Type,
        &lines_dict);
    if (!r) {
        return nullptr;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(lines_dict, &pos, &key, &value)) {
        double xs=0, ys=0, zs=0, xe=0, ye=0, ze=0, xt=0, yt=0, zt=0;
        int j = 0;


        char *motion_type = nullptr;
        int exec_mode = -1;
        int tool_no = -1;
        r = PyArg_ParseTuple(key, "sii", &motion_type, &exec_mode, &tool_no);
        if ((ParseExecMode)exec_mode == ParseExecMode::PREP) {
            // Prep moves are not part of the program and so should not affect the motion extents
            continue;
        }

        for(; j<PySequence_Length(value); j++) {
            PyObject *sj = PySequence_GetItem(value, j);
            PyObject *unused;
            int r;
            if(PyTuple_Size(sj) == 4)
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xt, &yt, &zt);
            else
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)O(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &unused, &xt, &yt, &zt);
            Py_DECREF(sj);
            if(!r) return nullptr;
            max_x = std::max(max_x, xs);
            max_y = std::max(max_y, ys);
            max_z = std::max(max_z, zs);
            min_x = std::min(min_x, xs);
            min_y = std::min(min_y, ys);
            min_z = std::min(min_z, zs);
            max_xt = std::max(max_xt, xs+xt);
            max_yt = std::max(max_yt, ys+yt);
            max_zt = std::max(max_zt, zs+zt);
            min_xt = std::min(min_xt, xs+xt);
            min_yt = std::min(min_yt, ys+yt);
            min_zt = std::min(min_zt, zs+zt);
        }
        if(j > 0) {
            max_x = std::max(max_x, xe);
            max_y = std::max(max_y, ye);
            max_z = std::max(max_z, ze);
            min_x = std::min(min_x, xe);
            min_y = std::min(min_y, ye);
            min_z = std::min(min_z, ze);
            max_xt = std::max(max_xt, xe+xt);
            max_yt = std::max(max_yt, ye+yt);
            max_zt = std::max(max_zt, ze+zt);
            min_xt = std::min(min_xt, xe+xt);
            min_yt = std::min(min_yt, ye+yt);
            min_zt = std::min(min_zt, ze+zt);
        }
    }
    return Py_BuildValue("[ddd][ddd][ddd][ddd]",
        min_x, min_y, min_z,  max_x, max_y, max_z,
        min_xt, min_yt, min_zt,  max_xt, max_yt, max_zt);
}


static PyObject *rs274_calc_extents(PyObject *self, PyObject *args) {
    double min_x = 9e99, min_y = 9e99, min_z = 9e99,
           min_xt = 9e99, min_yt = 9e99, min_zt = 9e99,
           max_x = -9e99, max_y = -9e99, max_z = -9e99,
           max_xt = -9e99, max_yt = -9e99, max_zt = -9e99;
    for(int i=0; i<PySequence_Length(args); i++) {
        PyObject *si = PyTuple_GetItem(args, i);
        if(!si) return nullptr;
        int j;
        double xs=0, ys=0, zs=0, xe=0, ye=0, ze=0, xt=0, yt=0, zt=0;
        for(j=0; j<PySequence_Length(si); j++) {
            PyObject *sj = PySequence_GetItem(si, j);
            PyObject *unused;
            int r;
            if(PyTuple_Size(sj) == 4)
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xt, &yt, &zt);
            else
                r = PyArg_ParseTuple(sj,
                    "O(dddOOOOOO)(dddOOOOOO)O(ddd):calc_extents item",
                    &unused,
                    &xs, &ys, &zs, &unused, &unused, &unused, &unused, &unused, &unused,
                    &xe, &ye, &ze, &unused, &unused, &unused, &unused, &unused, &unused,
                    &unused, &xt, &yt, &zt);
            Py_DECREF(sj);
            if(!r) return nullptr;
            max_x = std::max(max_x, xs);
            max_y = std::max(max_y, ys);
            max_z = std::max(max_z, zs);
            min_x = std::min(min_x, xs);
            min_y = std::min(min_y, ys);
            min_z = std::min(min_z, zs);
            max_xt = std::max(max_xt, xs+xt);
            max_yt = std::max(max_yt, ys+yt);
            max_zt = std::max(max_zt, zs+zt);
            min_xt = std::min(min_xt, xs+xt);
            min_yt = std::min(min_yt, ys+yt);
            min_zt = std::min(min_zt, zs+zt);
        }
        if(j > 0) {
            max_x = std::max(max_x, xe);
            max_y = std::max(max_y, ye);
            max_z = std::max(max_z, ze);
            min_x = std::min(min_x, xe);
            min_y = std::min(min_y, ye);
            min_z = std::min(min_z, ze);
            max_xt = std::max(max_xt, xe+xt);
            max_yt = std::max(max_yt, ye+yt);
            max_zt = std::max(max_zt, ze+zt);
            min_xt = std::min(min_xt, xe+xt);
            min_yt = std::min(min_yt, ye+yt);
            min_zt = std::min(min_zt, ze+zt);
        }
    }
    return Py_BuildValue("[ddd][ddd][ddd][ddd]",
        min_x, min_y, min_z,  max_x, max_y, max_z,
        min_xt, min_yt, min_zt,  max_xt, max_yt, max_zt);
}

#if PY_VERSION_HEX < 0x02050000
#define PyObject_GetAttrString(o,s) \
    PyObject_GetAttrString((o),const_cast<char*>((s)))
#define PyArg_VaParse(o,f,a) \
    PyArg_VaParse((o),const_cast<char*>((f)),(a))
#endif

static bool get_attr(PyObject *o, const char *attr_name, int *v) {
    PyObject *attr = PyObject_GetAttrString(o, attr_name);
    if(attr && PyInt_CheckAndError(attr_name, attr)) {
        *v = PyInt_AsLong(attr);
        Py_DECREF(attr);
        return true;
    }
    Py_XDECREF(attr);
    return false;
}

static bool get_attr(PyObject *o, const char *attr_name, double *v) {
    PyObject *attr = PyObject_GetAttrString(o, attr_name);
    if(attr && PyFloat_CheckAndError(attr_name, attr)) {
        *v = PyFloat_AsDouble(attr);
        Py_DECREF(attr);
        return true;
    }
    Py_XDECREF(attr);
    return false;
}

static bool get_attr(PyObject *o, const char *attr_name, const char *fmt, ...) {
    bool result = false;
    va_list ap;
    va_start(ap, fmt);
    PyObject *attr = PyObject_GetAttrString(o, attr_name);
    if(attr) result = PyArg_VaParse(attr, fmt, ap);
    va_end(ap);
    Py_XDECREF(attr);
    return result;
}

static void unrotate(double &x, double &y, double c, double s) {
    double tx = x * c + y * s;
    y = -x * s + y * c;
    x = tx;
}

static void rotate(double &x, double &y, double c, double s) {
    double tx = x * c - y * s;
    y = x * s + y * c;
    x = tx;
}

static PyObject *rs274_arc_to_segments(PyObject *self, PyObject *args) {
    PyObject *canon;
    double x1, y1, cx, cy, z1, a, b, c, u, v, w;
    double o[9], n[9], g5xoffset[9], g92offset[9];
    int rot, plane;
    int X, Y, Z;
    double rotation_cos, rotation_sin;
    int max_segments = 128;

    if(!PyArg_ParseTuple(args, "Oddddiddddddd|i:arcs_to_segments",
        &canon, &x1, &y1, &cx, &cy, &rot, &z1, &a, &b, &c, &u, &v, &w, &max_segments)) return nullptr;
    if(!get_attr(canon, "lo", "ddddddddd:arcs_to_segments lo", &o[0], &o[1], &o[2],
                    &o[3], &o[4], &o[5], &o[6], &o[7], &o[8]))
        return nullptr;
    if(!get_attr(canon, "plane", &plane)) return nullptr;
    if(!get_attr(canon, "rotation_cos", &rotation_cos)) return nullptr;
    if(!get_attr(canon, "rotation_sin", &rotation_sin)) return nullptr;
    if(!get_attr(canon, "g5x_offset_x", &g5xoffset[0])) return nullptr;
    if(!get_attr(canon, "g5x_offset_y", &g5xoffset[1])) return nullptr;
    if(!get_attr(canon, "g5x_offset_z", &g5xoffset[2])) return nullptr;
    if(!get_attr(canon, "g5x_offset_a", &g5xoffset[3])) return nullptr;
    if(!get_attr(canon, "g5x_offset_b", &g5xoffset[4])) return nullptr;
    if(!get_attr(canon, "g5x_offset_c", &g5xoffset[5])) return nullptr;
    if(!get_attr(canon, "g5x_offset_u", &g5xoffset[6])) return nullptr;
    if(!get_attr(canon, "g5x_offset_v", &g5xoffset[7])) return nullptr;
    if(!get_attr(canon, "g5x_offset_w", &g5xoffset[8])) return nullptr;
    if(!get_attr(canon, "g92_offset_x", &g92offset[0])) return nullptr;
    if(!get_attr(canon, "g92_offset_y", &g92offset[1])) return nullptr;
    if(!get_attr(canon, "g92_offset_z", &g92offset[2])) return nullptr;
    if(!get_attr(canon, "g92_offset_a", &g92offset[3])) return nullptr;
    if(!get_attr(canon, "g92_offset_b", &g92offset[4])) return nullptr;
    if(!get_attr(canon, "g92_offset_c", &g92offset[5])) return nullptr;
    if(!get_attr(canon, "g92_offset_u", &g92offset[6])) return nullptr;
    if(!get_attr(canon, "g92_offset_v", &g92offset[7])) return nullptr;
    if(!get_attr(canon, "g92_offset_w", &g92offset[8])) return nullptr;

    if(plane == 1) {
        X=0; Y=1; Z=2;
    } else if(plane == 3) {
        X=2; Y=0; Z=1;
    } else {
        X=1; Y=2; Z=0;
    }
    n[X] = x1;
    n[Y] = y1;
    n[Z] = z1;
    n[3] = a;
    n[4] = b;
    n[5] = c;
    n[6] = u;
    n[7] = v;
    n[8] = w;
    for(int ax=0; ax<9; ax++) o[ax] -= g5xoffset[ax];
    unrotate(o[0], o[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) o[ax] -= g92offset[ax];

    double theta1 = atan2(o[Y]-cy, o[X]-cx);
    double theta2 = atan2(n[Y]-cy, n[X]-cx);

    if(rot < 0) {
        while(theta2 - theta1 > -CIRCLE_FUZZ) theta2 -= 2*M_PI;
    } else {
        while(theta2 - theta1 < CIRCLE_FUZZ) theta2 += 2*M_PI;
    }

    // if multi-turn, add the right number of full circles
    if(rot < -1) theta2 += 2*M_PI*(rot+1);
    if(rot > 1) theta2 += 2*M_PI*(rot-1);

    int steps = std::max(3, int(max_segments * fabs(theta1 - theta2) / M_PI));
    double rsteps = 1. / steps;
    PyObject *segs = PyList_New(steps);

    double dtheta = theta2 - theta1;
    double d[9] = {0, 0, 0, n[3]-o[3], n[4]-o[4], n[5]-o[5], n[6]-o[6], n[7]-o[7], n[8]-o[8]};
    d[Z] = n[Z] - o[Z];

    double tx = o[X] - cx, ty = o[Y] - cy, dc = cos(dtheta*rsteps), ds = sin(dtheta*rsteps);
    for(int i=0; i<steps-1; i++) {
        double f = (i+1) * rsteps;
        double p[9];
        rotate(tx, ty, dc, ds);
        p[X] = tx + cx;
        p[Y] = ty + cy;
        p[Z] = o[Z] + d[Z] * f;
        p[3] = o[3] + d[3] * f;
        p[4] = o[4] + d[4] * f;
        p[5] = o[5] + d[5] * f;
        p[6] = o[6] + d[6] * f;
        p[7] = o[7] + d[7] * f;
        p[8] = o[8] + d[8] * f;
        for(int ax=0; ax<9; ax++) p[ax] += g92offset[ax];
        rotate(p[0], p[1], rotation_cos, rotation_sin);
        for(int ax=0; ax<9; ax++) p[ax] += g5xoffset[ax];
        PyList_SET_ITEM(segs, i,
            Py_BuildValue("ddddddddd", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8]));
    }
    for(int ax=0; ax<9; ax++) n[ax] += g92offset[ax];
    rotate(n[0], n[1], rotation_cos, rotation_sin);
    for(int ax=0; ax<9; ax++) n[ax] += g5xoffset[ax];
    PyList_SET_ITEM(segs, steps-1,
        Py_BuildValue("ddddddddd", n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8]));
    return segs;
}

static PyObject *get_set_max_glcanon_lines(PyObject *self, PyObject *args) {
    uint32_t in = 0;

    // The proper format arg is I which converts between
    // an unsigned int and a Python integer.
    // Formerly the code was using l which on the C++ side
    // is a long int which is 64-bits.  With some compiler optimization settings,
	// this can corrupt the stack return address and crash.

    if (PyArg_ParseTuple(args, "|I", &in) && in > 0) {
	// Found integer arg; set value
        max_glcanon_lines = in;
    }

    // return value
    return Py_BuildValue("I", max_glcanon_lines);
}

static PyMethodDef gcode_methods[] = {
    {"parse", (PyCFunction)parse_file, METH_VARARGS | METH_KEYWORDS, "Parse a G-Code file"},
    {"strerror", (PyCFunction)rs274_strerror, METH_VARARGS,
        "Convert a numeric error to a string"},
    {"calc_extents", (PyCFunction)rs274_calc_extents, METH_VARARGS,
        "Calculate information about extents of gcode"},
    {"calc_extents_dict", (PyCFunction)rs274_calc_extents_dict, METH_VARARGS,
        "Calculate information about extents of gcode using the dict of lines"},
    {"arc_to_segments", (PyCFunction)rs274_arc_to_segments, METH_VARARGS,
        "Convert an arc to straight segments"},
    {"max_glcanon_lines", (PyCFunction)get_set_max_glcanon_lines, METH_VARARGS,
     "Get maximum number of lines to preview; pass integer argument to set"},
    {"get_work_offsets_used", (PyCFunction)get_work_offsets_used, METH_VARARGS,
     "Get set of work offset integers used by last parsed gcode file"},
    {}
};

PyMODINIT_FUNC
initgcode(void) {
    PyObject *m = Py_InitModule3("gcode", gcode_methods,
                "Interface to EMC rs274ngc interpreter");
    PyType_Ready(&LineCodeType);
    PyModule_AddObject(m, "linecode", (PyObject*)&LineCodeType);
    PyObject_SetAttrString(m, "MAX_ERROR", PyInt_FromLong(maxerror));
    PyObject_SetAttrString(m, "MIN_ERROR",
            PyInt_FromLong(INTERP_MIN_ERROR));
    PyModule_AddIntConstant(m, "PARSE_EXEC_SEEK", (long)ParseExecMode::SEEK);
    PyModule_AddIntConstant(m, "PARSE_EXEC_PREP", (long)ParseExecMode::PREP);
    PyModule_AddIntConstant(m, "PARSE_EXEC_RUN", (long)ParseExecMode::RUN);
}

// vim:ts=8:sts=4:sw=4:et:
