//    This is a component of AXIS, a front-end for LinuxCNC
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

#define __STDC_FORMAT_MACROS
#include <Python.h>
#include <structseq.h>
#include <pthread.h>
#include <structmember.h>
#include <inttypes.h>
#include "config.h"
#include "rcs.hh"
#include "emc.hh"
#include "emc_nml.hh"
#include "kinematics.h"
#include "config.h"
#include "inifile.hh"
#include "timer.hh"
#include "nml_oi.hh"
#include "rcs_print.hh"
#include "tool_motion_data.hh"
#include "previewkinematics.hh"

#include <memory>
#include <algorithm>
using std::max;
using std::min;

#include <cmath>
#include <tuple>
#include <mutex>
#include <map>
#include <vector>
#include <set>

#ifndef T_BOOL
// The C++ standard probably doesn't specify the amount of storage for a 'bool',
// and on some systems it might be more than one byte.  However, on x86 and
// x86-64, sizeof(bool) == 1.  When a counterexample is found, this must be
// replaced with the result of a configure test.
#define T_BOOL T_UBYTE
#endif

#define NUM_AXES (9)

#define LOCAL_SPINDLE_FORWARD (1)
#define LOCAL_SPINDLE_REVERSE (-1)
#define LOCAL_SPINDLE_OFF (0)
#define LOCAL_SPINDLE_INCREASE (10)
#define LOCAL_SPINDLE_DECREASE (11)
#define LOCAL_SPINDLE_CONSTANT (12)

#define LOCAL_MIST_ON (1)
#define LOCAL_MIST_OFF (0)

#define LOCAL_FLOOD_ON (1)
#define LOCAL_FLOOD_OFF (0)

#define LOCAL_BRAKE_ENGAGE (1)
#define LOCAL_BRAKE_RELEASE (0)

#define LOCAL_JOG_STOP (0)
#define LOCAL_JOG_CONTINUOUS (1)
#define LOCAL_JOG_INCREMENT (2)

#define LOCAL_AUTO_RUN (0)
#define LOCAL_AUTO_PAUSE (1)
#define LOCAL_AUTO_RESUME (2)
#define LOCAL_AUTO_STEP (3)
#define LOCAL_AUTO_REVERSE (4)
#define LOCAL_AUTO_FORWARD (5)

/* This definition of offsetof avoids the g++ warning
 * 'invalid offsetof from non-POD type'.
 */
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

struct pyIniFile {
    PyObject_HEAD
    IniFile *i;
};

struct pyStatChannel {
    PyObject_HEAD
    RCS_STAT_CHANNEL *c;
    EMC_STAT status;
};

struct pyCommandChannel {
    PyObject_HEAD
    RCS_CMD_CHANNEL *c;
    RCS_STAT_CHANNEL *s;
    int serial;
};

struct pyErrorChannel {
    PyObject_HEAD
    NML *c;
};

static PyObject *m = NULL, *error = NULL;

static int Ini_init(pyIniFile *self, PyObject *a, PyObject *k) {
    char *inifile;
    if(!PyArg_ParseTuple(a, "s", &inifile)) return -1;

    if(!self->i)
        self->i = new IniFile();

    if (!self->i->Open(inifile)) {
        PyErr_Format( error, "inifile.open() failed");
        return -1;
    }
    return 0;
}

static PyObject *Ini_find(pyIniFile *self, PyObject *args) {
    const char *s1, *s2, *out;
    int num = 1; 
    if(!PyArg_ParseTuple(args, "ss|i:find", &s1, &s2, &num)) return NULL;
    
    out = self->i->Find(s2, s1, num);
    if(out == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyString_FromString(const_cast<char*>(out));
}

static PyObject *Ini_findall(pyIniFile *self, PyObject *args) {
    const char *s1, *s2, *out;
    int num = 1; 
    if(!PyArg_ParseTuple(args, "ss:findall", &s1, &s2)) return NULL;
    
    PyObject *result = PyList_New(0);
    while(1) {
        out = self->i->Find(s2, s1, num);
        if(out == NULL) {
            break;
        }
        PyList_Append(result, PyString_FromString(const_cast<char*>(out)));
        num++;
    }
    return result;
}

static void Ini_dealloc(pyIniFile *self) {
    if (self->i)
        self->i->Close();
    delete self->i;
    PyObject_Del(self);
}

static PyMethodDef Ini_methods[] = {
    {"find", (PyCFunction)Ini_find, METH_VARARGS,
        "Find value in inifile as string.  This uses the ConfigParser-style "
        "(section,option) order, not the linuxcnc order."},
    {"findall", (PyCFunction)Ini_findall, METH_VARARGS,
        "Find value in inifile as a list.  This uses the ConfigParser-style "
        "(section,option) order, not the linuxcnc order."},
    {NULL}
};

static PyTypeObject Ini_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "linuxcnc.ini",              /*tp_name*/
    sizeof(pyIniFile),      /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Ini_dealloc,/*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Ini_methods,            /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)Ini_init,     /*tp_init*/
    0,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

#define EMC_COMMAND_TIMEOUT 5.0  // how long to wait until timeout
#define EMC_COMMAND_DELAY   0.01 // how long to sleep between checks

static int emcWaitCommandComplete(pyCommandChannel *s, double timeout) {
    double start = etime();

    do {
        double now = etime();
        if(s->s->peek() == EMC_STAT_TYPE) {
           EMC_STAT *stat = (EMC_STAT*)s->s->get_address();
           int serial_diff = stat->echo_serial_number - s->serial;
           if (serial_diff > 0) {
                return RCS_DONE;
           }
           if (serial_diff == 0 &&
               ( stat->status == RCS_DONE || stat->status == RCS_ERROR )) {
                return stat->status;
           }
        }
        esleep(fmin(timeout - (now - start), EMC_COMMAND_DELAY));
    } while (etime() - start < timeout);
    return -1;
}

static int emcSendCommand(pyCommandChannel *s, RCS_CMD_MSG & cmd) {
    if (s->c->write(&cmd)) {
        return -1;
    }
    s->serial = cmd.serial_number;

    double start = etime();
    while (etime() - start < EMC_COMMAND_TIMEOUT) {
        EMC_STAT *stat = (EMC_STAT*)s->s->get_address();
        int serial_diff = stat->echo_serial_number - s->serial;
        if(s->s->peek() == EMC_STAT_TYPE &&
           serial_diff >= 0) {
                return 0;
           }
        esleep(EMC_COMMAND_DELAY);
    }
    return -1;
}

static char *get_nmlfile(void) {
    PyObject *fileobj = PyObject_GetAttrString(m, "nmlfile");
    if(fileobj == NULL) return NULL;
    return PyString_AsString(fileobj);
}

static int Stat_init(pyStatChannel *self, PyObject *a, PyObject *k) {
    char *file = get_nmlfile();
    if(file == NULL) return -1;

    RCS_STAT_CHANNEL *c =
        new RCS_STAT_CHANNEL(emcFormat, "emcStatus", "xemc", file);
    if(!c) {
        PyErr_Format( error, "new RCS_STAT_CHANNEL failed");
        return -1;
    }

    self->c = c;
    return 0;
}

static void Stat_dealloc(PyObject *self) {
    delete ((pyStatChannel*)self)->c;
    PyObject_Del(self);
}

static bool check_stat(RCS_STAT_CHANNEL *emcStatusBuffer) {
    if(!emcStatusBuffer->valid()) { 
        PyErr_Format( error, "emcStatusBuffer invalid err=%d", emcStatusBuffer->error_type);
        return false;
    }
    return true;
}

static PyObject *poll(pyStatChannel *s, PyObject *o) {
    if(!check_stat(s->c)) return NULL;
    if(s->c->peek() == EMC_STAT_TYPE) {
        EMC_STAT *emcStatus = static_cast<EMC_STAT*>(s->c->get_address());
        s->status = *emcStatus;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef Stat_methods[] = {
    {"poll", (PyCFunction)poll, METH_NOARGS, "Update current machine state"},
    {NULL}
};

#define O(x) offsetof(pyStatChannel,status.x)
static PyMemberDef Stat_members[] = {
// stat 
    {(char*)"echo_serial_number", T_INT, O(echo_serial_number), READONLY},
    {(char*)"state", T_INT, O(status), READONLY},

// task
    {(char*)"task_mode", T_INT, O(task.mode), READONLY},
    {(char*)"task_state", T_INT, O(task.state), READONLY},
    {(char*)"exec_state", T_INT, O(task.execState), READONLY},
    {(char*)"interp_state", T_INT, O(task.interpState), READONLY},
    {(char*)"call_level", T_INT, O(task.callLevel), READONLY},
    {(char*)"read_line", T_INT, O(task.readLine), READONLY},
    {(char*)"motion_line", T_INT, O(task.motionLine), READONLY},
    {(char*)"current_line", T_INT, O(task.currentLine), READONLY},
    {(char*)"previous_line", T_INT, O(task.previousLine), READONLY},
    {(char*)"recovery_line", T_INT, O(task.recoveryLine), READONLY},
    {(char*)"file", T_STRING_INPLACE, O(task.file), READONLY},
    {(char*)"working_dir", T_STRING_INPLACE, O(task.working_dir), READONLY},
    {(char*)"command", T_STRING_INPLACE, O(task.command), READONLY},
    {(char*)"program_units", T_INT, O(task.programUnits), READONLY},
    {(char*)"interpreter_errcode", T_INT, O(task.interpreter_errcode), READONLY},
    {(char*)"program_ended_and_reset", T_BOOL, O(task.program_ended_and_reset), READONLY},
    {(char*)"program_ended", T_BOOL, O(task.program_ended), READONLY},
    {(char*)"optional_stop", T_BOOL, O(task.optional_stop_state), READONLY},
    {(char*)"block_delete", T_BOOL, O(task.block_delete_state), READONLY},
    {(char*)"task_paused", T_INT, O(task.task_paused), READONLY},
    {(char*)"input_timeout", T_BOOL, O(task.input_timeout), READONLY},
    {(char*)"orient_timeout", T_BOOL, O(task.orient_timeout), READONLY},
    {(char*)"orient_no_exec_error", T_BOOL, O(task.orient_no_exec_error), READONLY},
    {(char*)"motion_tolerance", T_DOUBLE, O(task.motionTolerance), READONLY},
    {(char*)"naivecam_tolerance", T_DOUBLE, O(task.naivecamTolerance), READONLY},
    {(char*)"parts_counter_m30a", T_INT, O(task.part_counters[COUNTER_M_30_A]), READONLY},
    {(char*)"parts_counter_m30b", T_INT, O(task.part_counters[COUNTER_M_30_B]), READONLY},
    {(char*)"parts_counter_m99a", T_INT, O(task.part_counters[COUNTER_M_99_A]), READONLY},
    {(char*)"parts_counter_m99b", T_INT, O(task.part_counters[COUNTER_M_99_B]), READONLY},
    {(char*)"delay_left", T_DOUBLE, O(task.delayLeft), READONLY},
    {(char*)"queued_mdi_commands", T_INT, O(task.queuedMDIcommands), READONLY},

// motion
//   EMC_TRAJ_STAT traj
    {(char*)"linear_units", T_DOUBLE, O(motion.traj.linearUnits), READONLY},
    {(char*)"angular_units", T_DOUBLE, O(motion.traj.angularUnits), READONLY},
    {(char*)"cycle_time", T_DOUBLE, O(motion.traj.cycleTime), READONLY},
    {(char*)"axes", T_INT, O(motion.traj.axes), READONLY},
    {(char*)"axis_mask", T_INT, O(motion.traj.axis_mask), READONLY},
    {(char*)"motion_mode", T_INT, O(motion.traj.mode), READONLY},
    {(char*)"enabled", T_BOOL, O(motion.traj.enabled), READONLY},
    {(char*)"inpos", T_BOOL, O(motion.traj.inpos), READONLY},
    {(char*)"queue", T_INT, O(motion.traj.queue), READONLY},
    {(char*)"active_queue", T_INT, O(motion.traj.activeQueue), READONLY},
    {(char*)"queue_full", T_BOOL, O(motion.traj.queueFull), READONLY},
    {(char*)"id", T_INT, O(motion.traj.id), READONLY},
    {(char*)"next_id", T_INT, O(motion.traj.next_id), READONLY},
    {(char*)"completed_id", T_INT, O(motion.traj.completed_id), READONLY},
    {(char*)"paused", T_BOOL, O(motion.traj.paused), READONLY},
    {(char*)"stopped", T_BOOL, O(motion.traj.stopped), READONLY},
    {(char*)"feedrate", T_DOUBLE, O(motion.traj.scale), READONLY},
    {(char*)"rapidrate", T_DOUBLE, O(motion.traj.rapid_scale), READONLY},
    {(char*)"spindlerate", T_DOUBLE, O(motion.traj.spindle_scale), READONLY},
    
    {(char*)"velocity", T_DOUBLE, O(motion.traj.velocity), READONLY},
    {(char*)"acceleration", T_DOUBLE, O(motion.traj.acceleration), READONLY},
    {(char*)"max_velocity", T_DOUBLE, O(motion.traj.maxVelocity), READONLY},
    {(char*)"max_angular_velocity", T_DOUBLE,
     O(motion.traj.maxAngularVelocity), READONLY},
    {(char*)"max_acceleration", T_DOUBLE, O(motion.traj.maxAcceleration), READONLY},
    {(char*)"probe_tripped", T_BOOL, O(motion.traj.probe_tripped), READONLY},
    {(char*)"spindle_sync", T_DOUBLE, O(motion.traj.spindle_sync), READONLY},
    {(char*)"waiting_on_spindle", T_BOOL, O(motion.traj.waiting_on_spindle),
     READONLY},
    {(char*)"uu_per_rev", T_DOUBLE, O(motion.traj.uu_per_rev), READONLY},
    {(char*)"spindle_max_acceleration_rps2", T_DOUBLE, O(motion.traj.spindle_max_acceleration_rps2), READONLY},
    {(char*)"spindle_cmd_latency_sec", T_DOUBLE, O(motion.traj.spindle_cmd_latency_sec), READONLY},
    {(char*)"probing", T_BOOL, O(motion.traj.probing), READONLY},
    {(char*)"probe_val", T_INT, O(motion.traj.probeval), READONLY},
    {(char*)"setter_val", T_INT, O(motion.traj.setterval), READONLY},
    {(char*)"kinematics_type", T_INT, O(motion.traj.kinematics_type), READONLY},
    {(char*)"motion_type", T_INT, O(motion.traj.motion_type), READONLY},
    {(char*)"distance_to_go", T_DOUBLE, O(motion.traj.distance_to_go), READONLY},
    {(char*)"current_vel", T_DOUBLE, O(motion.traj.current_vel), READONLY},
    {(char*)"dwell_time_remaining", T_DOUBLE, O(motion.traj.dwell_time_remaining), READONLY},
    {(char*)"feed_override_enabled", T_BOOL, O(motion.traj.feed_override_enabled), READONLY},
    {(char*)"spindle_override_enabled", T_BOOL, O(motion.traj.spindle_override_enabled), READONLY},
    {(char*)"adaptive_feed_enabled", T_BOOL, O(motion.traj.adaptive_feed_enabled), READONLY},
    {(char*)"feed_hold_enabled", T_BOOL, O(motion.traj.feed_hold_enabled), READONLY},

// EMC_SPINDLE_STAT motion.spindle
    {(char*)"spindle_speed", T_DOUBLE, O(motion.spindle.speed), READONLY},
    {(char*)"spindle_direction", T_INT, O(motion.spindle.direction), READONLY},
    {(char*)"spindle_brake", T_INT, O(motion.spindle.brake), READONLY},
    {(char*)"spindle_increasing", T_INT, O(motion.spindle.increasing), READONLY},
    {(char*)"spindle_enabled", T_INT, O(motion.spindle.enabled), READONLY},

// io
// EMC_TOOL_STAT io.tool
    {(char*)"pocket_prepped", T_INT, O(io.tool.pocketPrepped), READONLY},
    {(char*)"tool_in_spindle", T_INT, O(io.tool.toolInSpindle), READONLY},

// EMC_COOLANT_STAT io.cooland
    {(char*)"mist", T_INT, O(io.coolant.mist), READONLY},
    {(char*)"flood", T_INT, O(io.coolant.flood), READONLY},

// EMC_AUX_STAT     io.aux
    {(char*)"estop", T_INT, O(io.aux.estop), READONLY},

// EMC_LUBE_STAT    io.lube
    {(char*)"lube", T_INT, O(io.lube.on), READONLY},
    {(char*)"lube_level", T_INT, O(io.lube.level), READONLY},

    {(char*)"debug", T_INT, O(debug), READONLY},
    {NULL}
};

static PyObject *int_array(int const *arr, int sz) {
    PyObject *res = PyTuple_New(sz);
    for(int i = 0; i < sz; i++) {
        PyTuple_SET_ITEM(res, i, PyInt_FromLong(arr[i]));
    }
    return res;
}

static PyObject *double_array(double const *arr, int sz) {
    PyObject *res = PyTuple_New(sz);
    for(int i = 0; i < sz; i++) {
        PyTuple_SET_ITEM(res, i, PyFloat_FromDouble(arr[i]));
    }
    return res;
}

static PyObject *pose(const EmcPose &p) {
    PyObject *res = PyTuple_New(9);
    PyTuple_SET_ITEM(res, 0, PyFloat_FromDouble(p.tran.x));
    PyTuple_SET_ITEM(res, 1, PyFloat_FromDouble(p.tran.y));
    PyTuple_SET_ITEM(res, 2, PyFloat_FromDouble(p.tran.z));
    PyTuple_SET_ITEM(res, 3, PyFloat_FromDouble(p.a));
    PyTuple_SET_ITEM(res, 4, PyFloat_FromDouble(p.b));
    PyTuple_SET_ITEM(res, 5, PyFloat_FromDouble(p.c));
    PyTuple_SET_ITEM(res, 6, PyFloat_FromDouble(p.u));
    PyTuple_SET_ITEM(res, 7, PyFloat_FromDouble(p.v));
    PyTuple_SET_ITEM(res, 8, PyFloat_FromDouble(p.w));
    return res;
}

static PyObject *tupleFromRotateShiftTransform(const RotateShiftTransform &p) {
    PyObject *res = PyTuple_New(10);

    for (int i = 0; i <9; ++i) {
        PyTuple_SET_ITEM(res, i, PyFloat_FromDouble(p.shift.tf[i]));
    }
    PyTuple_SET_ITEM(res, 9, PyFloat_FromDouble(p.getXYRotationDeg()));
    return res;
}

static PyObject *Stat_g5x_index(pyStatChannel *s) {
    return PyInt_FromLong(s->status.task.g5x_index);
}

static PyObject *Stat_g5x_offset(pyStatChannel *s) {
    PyObject *res = PyTuple_New(9);

    PositionArray const &p = s->status.task.g5x_offset();
    for (int i = 0; i <9; ++i) {
        PyTuple_SET_ITEM(res, i, PyFloat_FromDouble(p[i]));
    }
    return res;
}

static PyObject *Stat_rotation_xy(pyStatChannel *s) {
    return PyFloat_FromDouble(s->status.task.rotation_xy());
}

static PyObject *Stat_g5x_offsets(pyStatChannel *s) {
    PyObject *res = PyDict_New();
    if (!res) {return res;}
    // Set index 0 to the current offset so that we can write
    // `stat.g5x_offsets[stat.g5x_index]` in python
    PyObject *k0 = PyInt_FromLong(0);
    PyObject *v0 = tupleFromRotateShiftTransform(s->status.task.g5x_table[s->status.task.g5x_index]);
    PyDict_SetItem(res, k0, v0);
    Py_XDECREF(k0);
    Py_XDECREF(v0);

    for (size_t i = 1; i < s->status.task.g5x_table.size(); ++i) {
        PyObject *key = PyInt_FromSize_t(i);
        PyObject *val = tupleFromRotateShiftTransform(s->status.task.g5x_table[i]);

        PyDict_SetItem(res, key, val);
        Py_XDECREF(key);
        Py_XDECREF(val);
    }
    return res;
}

static PyObject *Stat_g92_offset(pyStatChannel *s) {
    return pose(s->status.task.g92_offset);
}

static PyObject *Stat_tool_offset(pyStatChannel *s) {
    return pose(s->status.task.toolOffset);
}

static PyObject *Stat_position(pyStatChannel *s) {
    return pose(s->status.motion.traj.position);
}

static PyObject *Stat_dtg(pyStatChannel *s) {
    return pose(s->status.motion.traj.dtg);
}

static PyObject *Stat_actual(pyStatChannel *s) {
    return pose(s->status.motion.traj.actualPosition);
}

static PyObject *Stat_joint_position(pyStatChannel *s) {
    PyObject *res = PyTuple_New(EMC_AXIS_MAX);
    for(int i=0; i<EMC_AXIS_MAX; i++) {
        PyTuple_SetItem(res, i,
                PyFloat_FromDouble(s->status.motion.axis[i].output));
    }
    return res;
}

static PyObject *Stat_joint_actual(pyStatChannel *s) {
    PyObject *res = PyTuple_New(EMC_AXIS_MAX);
    for(int i=0; i<EMC_AXIS_MAX; i++) {
        PyTuple_SetItem(res, i,
                PyFloat_FromDouble(s->status.motion.axis[i].input));
    }
    return res;
}

static PyObject *Stat_probed(pyStatChannel *s) {
    return pose(s->status.motion.traj.probedPosition);
}

static PyObject *Stat_activegcodes(pyStatChannel *s) {
    return int_array(s->status.task.modalState.g_codes.data(), ACTIVE_G_CODES);
}

static PyObject *Stat_activemcodes(pyStatChannel *s) {
    return int_array(s->status.task.modalState.m_codes.data(), ACTIVE_M_CODES);
}

static PyObject *Stat_activesettings(pyStatChannel *s) {
   return double_array(s->status.task.modalState.settings.data(), ACTIVE_SETTINGS);
}

static PyObject *Stat_din(pyStatChannel *s) {
    return int_array(s->status.motion.synch_di, EMC_MAX_AIO);
}

static PyObject *Stat_dout(pyStatChannel *s) {
    return int_array(s->status.motion.synch_do, EMC_MAX_AIO);
}

static PyObject *Stat_limit(pyStatChannel *s) {
    int sz = NUM_AXES;
    PyObject *res = PyTuple_New(sz);
    for(int i = 0; i < sz; i++) {
        int v = 0;
        if(s->status.motion.axis[i].minHardLimit) v |= 1;
        if(s->status.motion.axis[i].maxHardLimit) v |= 2;
        if(s->status.motion.axis[i].minSoftLimit) v |= 4;
        if(s->status.motion.axis[i].maxSoftLimit) v |= 8;
        PyTuple_SET_ITEM(res, i, PyInt_FromLong(v));
    }
    return res;
}

static PyObject *Stat_homed(pyStatChannel *s) {
    int sz = NUM_AXES;
    PyObject *res = PyTuple_New(sz);
    for(int i = 0; i < sz; i++) {
        PyTuple_SET_ITEM(res, i, PyInt_FromLong(s->status.motion.axis[i].homed));
    }
    return res;
}

static PyObject *Stat_ain(pyStatChannel *s) {
    return double_array(s->status.motion.analog_input, EMC_MAX_AIO);
}

static PyObject *Stat_aout(pyStatChannel *s) {
    return double_array(s->status.motion.analog_output, EMC_MAX_AIO);
}

static void dict_add(PyObject *d, const char *name, unsigned char v) {
    PyObject *o;
    PyDict_SetItemString(d, name, o = PyInt_FromLong(v));
    Py_XDECREF(o);
}
static void dict_add(PyObject *d, const char *name, double v) {
    PyObject *o;
    PyDict_SetItemString(d, name, o = PyFloat_FromDouble(v));
    Py_XDECREF(o);
}

#define F(x) F2(#x, x)
#define F2(y,x) dict_add(res, y, s->status.motion.axis[axisno].x)
static PyObject *Stat_axis_one(pyStatChannel *s, int axisno) {
    PyObject *res = PyDict_New();
    F(axisType);
    F(units);
    F(backlash);
    F2("min_position_limit", minPositionLimit);
    F2("max_position_limit", maxPositionLimit);
    F2("max_ferror", maxFerror);
    F2("min_ferror", minFerror);
    F2("ferror_current", ferrorCurrent);
    F2("ferror_highmark", ferrorHighMark);
    F(output);
    F(input);
    F(velocity);
    F(inpos);
    F(homing);
    F(homed);
    F(fault);
    F(enabled);
    F2("min_soft_limit", minSoftLimit);
    F2("max_soft_limit", maxSoftLimit);
    F2("min_hard_limit", minHardLimit);
    F2("max_hard_limit", maxHardLimit);
    F2("override_limits", overrideLimits);
    F(vel_limit);
    return res;
}
#undef F
#undef F2

static PyObject *Stat_axis(pyStatChannel *s) {
    PyObject *res = PyTuple_New(EMC_AXIS_MAX);
    for(int i=0; i<EMC_AXIS_MAX; i++) {
        PyTuple_SetItem(res, i, Stat_axis_one(s, i));
    }
    return res;
}

static PyObject *Stat_axis_limits(pyStatChannel *s) {
    PyObject *min_limit = PyTuple_New(EMC_AXIS_MAX);
    PyObject *max_limit = PyTuple_New(EMC_AXIS_MAX);

    for(int i=0; i<EMC_AXIS_MAX; i++) {
        PyTuple_SET_ITEM(min_limit, i, PyFloat_FromDouble(s->status.motion.axis[i].minPositionLimit));
        PyTuple_SET_ITEM(max_limit, i, PyFloat_FromDouble(s->status.motion.axis[i].maxPositionLimit));
    }
    PyObject *limit_pair = PyTuple_New(2);
    PyTuple_SET_ITEM(limit_pair, 0, min_limit);
    PyTuple_SET_ITEM(limit_pair, 1, max_limit);
    return limit_pair;
}

static PyStructSequence_Field tool_fields[] = {
    {(char*)"id", },
    {(char*)"xoffset", },
    {(char*)"yoffset", },
    {(char*)"zoffset", },
    {(char*)"aoffset", },
    {(char*)"boffset", },
    {(char*)"coffset", },
    {(char*)"uoffset", },
    {(char*)"voffset", },
    {(char*)"woffset", },
    {(char*)"diameter", },
    {(char*)"frontangle", },
    {(char*)"backangle", },
    {(char*)"orientation", },
    {0,},
};

static PyStructSequence_Desc tool_result_desc = {
    (char*)"tool_result", /* name */
    (char*)"", /* doc */
    tool_fields,
    14
};

static PyTypeObject ToolResultType;

static PyObject *Stat_tool_table(pyStatChannel *s) {
    PyObject *res = PyTuple_New(CANON_POCKETS_MAX);
    int j=0;
    for(int i=0; i<CANON_POCKETS_MAX; i++) {
        struct CANON_TOOL_TABLE &t = s->status.io.tool.toolTable[i];
        PyObject *tool = PyStructSequence_New(&ToolResultType);
        PyStructSequence_SET_ITEM(tool, 0, PyInt_FromLong(t.toolno));
        PyStructSequence_SET_ITEM(tool, 1, PyFloat_FromDouble(t.offset.tran.x));
        PyStructSequence_SET_ITEM(tool, 2, PyFloat_FromDouble(t.offset.tran.y));
        PyStructSequence_SET_ITEM(tool, 3, PyFloat_FromDouble(t.offset.tran.z));
        PyStructSequence_SET_ITEM(tool, 4, PyFloat_FromDouble(t.offset.a));
        PyStructSequence_SET_ITEM(tool, 5, PyFloat_FromDouble(t.offset.b));
        PyStructSequence_SET_ITEM(tool, 6, PyFloat_FromDouble(t.offset.c));
        PyStructSequence_SET_ITEM(tool, 7, PyFloat_FromDouble(t.offset.u));
        PyStructSequence_SET_ITEM(tool, 8, PyFloat_FromDouble(t.offset.v));
        PyStructSequence_SET_ITEM(tool, 9, PyFloat_FromDouble(t.offset.w));
        PyStructSequence_SET_ITEM(tool, 10, PyFloat_FromDouble(t.diameter));
        PyStructSequence_SET_ITEM(tool, 11, PyFloat_FromDouble(t.frontangle));
        PyStructSequence_SET_ITEM(tool, 12, PyFloat_FromDouble(t.backangle));
        PyStructSequence_SET_ITEM(tool, 13, PyInt_FromLong(t.orientation));
        PyTuple_SetItem(res, j, tool);
        j++;
    }
    _PyTuple_Resize(&res, j);
    return res;
}

// XXX io.tool.toolTable
// XXX EMC_AXIS_STAT motion.axis[]

static PyGetSetDef Stat_getsetlist[] = {
    {(char*)"actual_position", (getter)Stat_actual},
    {(char*)"ain", (getter)Stat_ain},
    {(char*)"aout", (getter)Stat_aout},
    {(char*)"axis", (getter)Stat_axis},
    {(char*)"axis_limits", (getter)Stat_axis_limits},
    {(char*)"din", (getter)Stat_din},
    {(char*)"dout", (getter)Stat_dout},
    {(char*)"gcodes", (getter)Stat_activegcodes},
    {(char*)"homed", (getter)Stat_homed},
    {(char*)"limit", (getter)Stat_limit},
    {(char*)"mcodes", (getter)Stat_activemcodes},
    {(char*)"g5x_offset", (getter)Stat_g5x_offset},
    {(char*)"rotation_xy", (getter)Stat_rotation_xy},
    {(char*)"g5x_offsets", (getter)Stat_g5x_offsets},
    {(char*)"g5x_index", (getter)Stat_g5x_index},
    {(char*)"g92_offset", (getter)Stat_g92_offset},
    {(char*)"position", (getter)Stat_position},
    {(char*)"dtg", (getter)Stat_dtg},
    {(char*)"joint_position", (getter)Stat_joint_position},
    {(char*)"joint_actual_position", (getter)Stat_joint_actual},
    {(char*)"probed_position", (getter)Stat_probed},
    {(char*)"settings", (getter)Stat_activesettings, (setter)NULL,
        (char*)"This is an array containing the Interp active settings: sequence number,\n"
        "feed rate, and spindle speed."
    },
    {(char*)"tool_offset", (getter)Stat_tool_offset},
    {(char*)"tool_table", (getter)Stat_tool_table},
    {NULL}
};

static PyTypeObject Stat_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "linuxcnc.stat",             /*tp_name*/
    sizeof(pyStatChannel),  /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Stat_dealloc, /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Stat_methods,           /*tp_methods*/
    Stat_members,           /*tp_members*/
    Stat_getsetlist,        /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)Stat_init,    /*tp_init*/
    0,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

static int Command_init(pyCommandChannel *self, PyObject *a, PyObject *k) {
    char *file = get_nmlfile();
    if(file == NULL) return -1;

    RCS_CMD_CHANNEL *c =
        new RCS_CMD_CHANNEL(emcFormat, "emcCommand", "xemc", file);
    if(!c) {
        PyErr_Format( error, "new RCS_CMD_CHANNEL failed");
        return -1;
    }
    RCS_STAT_CHANNEL *s =
        new RCS_STAT_CHANNEL(emcFormat, "emcStatus", "xemc", file);
    if(!c) {
	delete s;
        PyErr_Format( error, "new RCS_STAT_CHANNEL failed");
        return -1;
    }

    self->s = s;
    self->c = c;
    return 0;
}

static void Command_dealloc(PyObject *self) {
    delete ((pyCommandChannel*)self)->c;
    PyObject_Del(self);
}

static PyObject *block_delete(pyCommandChannel *s, PyObject *o) {
    int t;
    EMC_TASK_PLAN_SET_BLOCK_DELETE m;

    if(!PyArg_ParseTuple(o, "i", &t)) return NULL;
    m.state = t;
            
    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *optional_stop(pyCommandChannel *s, PyObject *o) {
    int t;
    EMC_TASK_PLAN_SET_OPTIONAL_STOP m;

    if(!PyArg_ParseTuple(o, "i", &t)) return NULL;
    m.state = t;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *mode(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_SET_MODE m;
    if(!PyArg_ParseTuple(o, "i", &m.mode)) return NULL;
    switch(m.mode) {
        case EMC_TASK_MODE_MDI:
        case EMC_TASK_MODE_MANUAL:
        case EMC_TASK_MODE_AUTO:
            break;
        default:
            PyErr_Format(PyExc_ValueError,"Mode should be MODE_MDI, MODE_MANUAL, or MODE_AUTO");
            return NULL;
    }
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_axis_max_velocity(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_SET_MAX_VELOCITY m;
    if(!PyArg_ParseTuple(o, "id", &m.axis, &m.vel)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *maxvel(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_MAX_VELOCITY m;
    if(!PyArg_ParseTuple(o, "dd", &m.velocity, &m.velocity_angular))
	return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *feedrate(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_SCALE m;
    if(!PyArg_ParseTuple(o, "d", &m.scale)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *rapidrate(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_RAPID_SCALE m;
    if(!PyArg_ParseTuple(o, "d", &m.scale)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *spindleoverride(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_SPINDLE_SCALE m;
    if(!PyArg_ParseTuple(o, "d", &m.scale)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *spindle(pyCommandChannel *s, PyObject *o) {
    int dir;
    double vel = 1;
    if(!PyArg_ParseTuple(o, "i|d", &dir, &vel)) return NULL;
    switch(dir) {
        case LOCAL_SPINDLE_FORWARD:
        case LOCAL_SPINDLE_REVERSE:
        {
            EMC_SPINDLE_ON m;
            m.speed = dir * vel;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_SPINDLE_INCREASE:
        {
            EMC_SPINDLE_INCREASE m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_SPINDLE_DECREASE:
        {
            EMC_SPINDLE_DECREASE m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_SPINDLE_CONSTANT:
        {
            EMC_SPINDLE_CONSTANT m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_SPINDLE_OFF:
        {
            EMC_SPINDLE_OFF m;
            emcSendCommand(s, m);
        }
            break;
        default:
            PyErr_Format(PyExc_ValueError,"Spindle direction should be SPINDLE_FORWARD, SPINDLE_REVERSE, SPINDLE_OFF, SPINDLE_INCREASE, SPINDLE_DECREASE, or SPINDLE_CONSTANT");
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *mdi(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_PLAN_EXECUTE m;
    char *cmd;
    int len;
    if(!PyArg_ParseTuple(o, "s#", &cmd, &len)) return NULL;
    if(unsigned(len) > sizeof(m.command) - 1) {
        PyErr_Format(PyExc_ValueError, "MDI commands limited to %zu characters", sizeof(m.command) - 1);
        return NULL;
    }
    strcpy(m.command, cmd);
    m.cmd_src = (int)CommandSource::MDI;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *state(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_SET_STATE m;
    if(!PyArg_ParseTuple(o, "i", &m.state)) return NULL;
    switch(m.state){
        case EMC_TASK_STATE_ESTOP:
        case EMC_TASK_STATE_ESTOP_RESET:
        case EMC_TASK_STATE_ON:
        case EMC_TASK_STATE_OFF:
            break;
        default:
            PyErr_Format(PyExc_ValueError,"Machine state should be STATE_ESTOP, STATE_ESTOP_RESET, STATE_ON, or STATE_OFF");
            return NULL;
    }
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *tool_offset(pyCommandChannel *s, PyObject *o) {
    EMC_TOOL_SET_OFFSET m;
    if(!PyArg_ParseTuple(o, "idddddi", &m.toolno, &m.offset.tran.z, &m.offset.tran.x, &m.diameter, 
                         &m.frontangle, &m.backangle, &m.orientation)) 
        return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *mist(pyCommandChannel *s, PyObject *o) {
    int dir;
    if(!PyArg_ParseTuple(o, "i", &dir)) return NULL;
    switch(dir) {
        case LOCAL_MIST_ON:
        {
            EMC_COOLANT_MIST_ON m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_MIST_OFF:
        {
            EMC_COOLANT_MIST_OFF m;
            emcSendCommand(s, m);
        }
            break;
        default:
            PyErr_Format(PyExc_ValueError,"Mist should be MIST_ON or MIST_OFF");
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *flood(pyCommandChannel *s, PyObject *o) {
    int dir;
    if(!PyArg_ParseTuple(o, "i", &dir)) return NULL;
    switch(dir) {
        case LOCAL_FLOOD_ON:
        {
            EMC_COOLANT_FLOOD_ON m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_FLOOD_OFF:
        {
            EMC_COOLANT_FLOOD_OFF m;
            emcSendCommand(s, m);
        }
            break;
        default:
            PyErr_Format(PyExc_ValueError,"FLOOD should be FLOOD_ON or FLOOD_OFF");
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *brake(pyCommandChannel *s, PyObject *o) {
    int dir;
    if(!PyArg_ParseTuple(o, "i", &dir)) return NULL;
    switch(dir) {
        case LOCAL_BRAKE_ENGAGE:
        {
            EMC_SPINDLE_BRAKE_ENGAGE m;
            emcSendCommand(s, m);
        }
            break;
        case LOCAL_BRAKE_RELEASE:
        {
            EMC_SPINDLE_BRAKE_RELEASE m;
            emcSendCommand(s, m);
        }
            break;
        default:
            PyErr_Format(PyExc_ValueError,"BRAKE should be BRAKE_ENGAGE or BRAKE_RELEASE");
            return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *load_tool_table(pyCommandChannel *s, PyObject *o) {
    EMC_TOOL_LOAD_TOOL_TABLE m;
    m.file[0] = '\0'; // don't override the ini file
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *emcabort(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_ABORT m;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *override_limits(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_OVERRIDE_LIMITS m;
    m.axis = 0; // same number for all
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *home(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_HOME m;
    if(!PyArg_ParseTuple(o, "i", &m.axis)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *unhome(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_UNHOME m;
    if(!PyArg_ParseTuple(o, "i", &m.axis)) return NULL;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

// jog(JOG_STOP, axis) 
// jog(JOG_CONTINUOUS, axis, speed) 
// jog(JOG_INCREMENT, axis, speed, increment)

static PyObject *jog(pyCommandChannel *s, PyObject *o) {
    int fn;
    int axis;
    double vel, inc;


    if(!PyArg_ParseTuple(o, "ii|dd", &fn, &axis, &vel, &inc)) return NULL;
    if(fn == LOCAL_JOG_STOP) {
        if(PyTuple_Size(o) != 2) {
            PyErr_Format( PyExc_TypeError,
                "jog(JOG_STOP, ...) takes 2 arguments (%lu given)",
                (unsigned long)PyTuple_Size(o));
            return NULL;
        }
        EMC_AXIS_ABORT abort;
        abort.axis = axis;
        emcSendCommand(s, abort);
    } else if(fn == LOCAL_JOG_CONTINUOUS) {
        if(PyTuple_Size(o) != 3) {
            PyErr_Format( PyExc_TypeError,
                "jog(JOG_CONTINUOUS, ...) takes 3 arguments (%lu given)",
                (unsigned long)PyTuple_Size(o));
            return NULL;
        }
        EMC_AXIS_JOG cont;
        cont.axis = axis;
        cont.vel = vel;
        emcSendCommand(s, cont);
    } else if(fn == LOCAL_JOG_INCREMENT) {
        if(PyTuple_Size(o) != 4) {
            PyErr_Format( PyExc_TypeError,
                "jog(JOG_INCREMENT, ...) takes 4 arguments (%lu given)",
                (unsigned long)PyTuple_Size(o));
            return NULL;
        }

        EMC_AXIS_INCR_JOG incr;
        incr.axis = axis;
        incr.vel = vel;
        incr.incr = inc;
        emcSendCommand(s, incr);
    } else {
        PyErr_Format( PyExc_TypeError, "jog() first argument must be JOG_xxx");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *reset_interpreter(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_PLAN_INIT m;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *program_open(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_PLAN_OPEN m{};
    char *file = nullptr;
    char *working_dir = nullptr;
    int len = 0;

    if(!PyArg_ParseTuple(o, "s#|s", &file, &len, &working_dir)) return nullptr;
    if(unsigned(len) > sizeof(m.file) - 1) {
        PyErr_Format(PyExc_ValueError, "File name limited to %zu characters", sizeof(m.file) - 1);
        return nullptr;
    }
    strcpy(m.file, file);
    if (working_dir) {
        strcpy(m.working_dir, working_dir);
    }
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *program_close(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_PLAN_CLOSE m;

    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *synch_and_export(pyCommandChannel *s, PyObject *o) {
    EMC_TASK_PLAN_SYNCH_EXPORT m;

    char *file = nullptr;
    int len = 0;

    if(!PyArg_ParseTuple(o, "s", &file, &len)) return nullptr;
    const size_t max_len = sizeof(m.export_file) - 1;
    if(unsigned(len) > max_len) {
        PyErr_Format(PyExc_ValueError, "File name limited to %zu characters", max_len);
        return nullptr;
    }
    //FIXME does file get deallocated here?
    strcpy(m.export_file, file);
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *clear_recovery_info(pyCommandChannel *s, PyObject *) {
    EMC_TASK_PLAN_CLEAR_RECOVERY_INFO m;
    emcSendCommand(s, m);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *emcauto(pyCommandChannel *s, PyObject *o) {
    int fn;
    EMC_TASK_PLAN_RUN run{};
    EMC_TASK_PLAN_PAUSE pause;
    EMC_TASK_PLAN_REVERSE reverse;
    EMC_TASK_PLAN_FORWARD forward;
    EMC_TASK_PLAN_RESUME resume;
    EMC_TASK_PLAN_STEP step;

    if(PyArg_ParseTuple(o, "ii|ii", &fn, &run.run_settings.start_line, &run.run_settings.leadin_type, &run.run_settings.keep_current_tool) && fn == LOCAL_AUTO_RUN) {
        emcSendCommand(s, run);
    } else {
        PyErr_Clear();
        if(!PyArg_ParseTuple(o, "i", &fn)) return NULL;
        switch(fn) {
        case LOCAL_AUTO_PAUSE:
            emcSendCommand(s, pause);
            break;
        case LOCAL_AUTO_RESUME:
            emcSendCommand(s, resume);
            break;
        case LOCAL_AUTO_STEP:
            emcSendCommand(s, step);
            break;
        case LOCAL_AUTO_REVERSE:
            emcSendCommand(s, reverse);
            break;
        case LOCAL_AUTO_FORWARD:
            emcSendCommand(s, forward);
            break;
        default:
            PyErr_Format(error, "Unexpected argument '%d' to command.auto", fn);
            return NULL;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *debug(pyCommandChannel *s, PyObject *o) {
    EMC_SET_DEBUG d;

    if(!PyArg_ParseTuple(o, "i", &d.debug)) return NULL;
    emcSendCommand(s, d);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *teleop(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_TELEOP_ENABLE en;

    if(!PyArg_ParseTuple(o, "i", &en.enable)) return NULL;

    emcSendCommand(s, en);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_traj_mode(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_MODE mo;

    if(!PyArg_ParseTuple(o, "i", &mo.mode)) return NULL;

    emcSendCommand(s, mo);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_teleop_vector(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_TELEOP_VECTOR mo;

    mo.vector.a = mo.vector.b = mo.vector.c = 0.;

    if(!PyArg_ParseTuple(o, "ddd|ddd", &mo.vector.tran.x, &mo.vector.tran.y, &mo.vector.tran.z, &mo.vector.a, &mo.vector.b, &mo.vector.c))
        return NULL;

    emcSendCommand(s, mo);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_min_limit(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_SET_MIN_POSITION_LIMIT m;
    if(!PyArg_ParseTuple(o, "id", &m.axis, &m.limit))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_max_limit(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_SET_MAX_POSITION_LIMIT m;
    if(!PyArg_ParseTuple(o, "id", &m.axis, &m.limit))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_feed_override(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_FO_ENABLE m;
    if(!PyArg_ParseTuple(o, "b", &m.mode))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_spindle_override(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_SO_ENABLE m;
    if(!PyArg_ParseTuple(o, "b", &m.mode))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_feed_hold(pyCommandChannel *s, PyObject *o) {
    EMC_TRAJ_SET_FH_ENABLE m;
    if(!PyArg_ParseTuple(o, "b", &m.mode))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_adaptive_feed(pyCommandChannel *s, PyObject *o) {
    EMC_MOTION_ADAPTIVE m;
    if(!PyArg_ParseTuple(o, "b", &m.status))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_digital_output(pyCommandChannel *s, PyObject *o) {
    EMC_MOTION_SET_DOUT m;
    if(!PyArg_ParseTuple(o, "bb", &m.index, &m.start))
        return NULL;

    m.now = 1;
    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_analog_output(pyCommandChannel *s, PyObject *o) {
    EMC_MOTION_SET_AOUT m;
    if(!PyArg_ParseTuple(o, "bd", &m.index, &m.start))
        return NULL;

    m.now = 1;
    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *set_homing_params(pyCommandChannel *s, PyObject *o) {
    EMC_AXIS_SET_HOMING_PARAMS m;
    if(!PyArg_ParseTuple(o, "idddddiiiiii", &m.axis, &m.home, &m.offset, &m.home_final_vel, 
        &m.search_vel, &m.latch_vel, &m.use_index, &m.ignore_limits, &m.is_shared, 
        &m.home_sequence, &m.volatile_home, &m.locking_indexer))
        return NULL;

    emcSendCommand(s, m);

    Py_INCREF(Py_None);
    return Py_None;
}                                        

static PyObject *wait_complete(pyCommandChannel *s, PyObject *o) {
    double timeout = EMC_COMMAND_TIMEOUT;
    if (!PyArg_ParseTuple(o, "|d:emc.command.wait_complete", &timeout))
        return NULL;
    return PyInt_FromLong(emcWaitCommandComplete(s, timeout));
}





static PyMemberDef Command_members[] = {
    {(char*)"serial", T_INT, offsetof(pyCommandChannel, serial), READONLY},
    {NULL}
};

static PyMethodDef Command_methods[] = {
    {"debug", (PyCFunction)debug, METH_VARARGS},
    {"teleop_enable", (PyCFunction)teleop, METH_VARARGS},
    {"teleop_vector", (PyCFunction)set_teleop_vector, METH_VARARGS},
    {"traj_mode", (PyCFunction)set_traj_mode, METH_VARARGS},
    {"wait_complete", (PyCFunction)wait_complete, METH_VARARGS},
    {"state", (PyCFunction)state, METH_VARARGS},
    {"mdi", (PyCFunction)mdi, METH_VARARGS},
    {"mode", (PyCFunction)mode, METH_VARARGS},
    {"feedrate", (PyCFunction)feedrate, METH_VARARGS},
    {"rapidrate", (PyCFunction)rapidrate, METH_VARARGS},
    {"maxvel", (PyCFunction)maxvel, METH_VARARGS},
    {"spindleoverride", (PyCFunction)spindleoverride, METH_VARARGS},
    {"spindle", (PyCFunction)spindle, METH_VARARGS},
    {"tool_offset", (PyCFunction)tool_offset, METH_VARARGS},
    {"mist", (PyCFunction)mist, METH_VARARGS},
    {"flood", (PyCFunction)flood, METH_VARARGS},
    {"brake", (PyCFunction)brake, METH_VARARGS},
    {"load_tool_table", (PyCFunction)load_tool_table, METH_NOARGS},
    {"abort", (PyCFunction)emcabort, METH_NOARGS},
    {"override_limits", (PyCFunction)override_limits, METH_NOARGS},
    {"home", (PyCFunction)home, METH_VARARGS},
    {"unhome", (PyCFunction)unhome, METH_VARARGS},
    {"jog", (PyCFunction)jog, METH_VARARGS},
    {"reset_interpreter", (PyCFunction)reset_interpreter, METH_NOARGS},
    {"program_open", (PyCFunction)program_open, METH_VARARGS},
    {"auto", (PyCFunction)emcauto, METH_VARARGS},
    {"set_optional_stop", (PyCFunction)optional_stop, METH_VARARGS},
    {"set_block_delete", (PyCFunction)block_delete, METH_VARARGS},
    {"set_min_limit", (PyCFunction)set_min_limit, METH_VARARGS},
    {"set_max_limit", (PyCFunction)set_max_limit, METH_VARARGS},
    {"set_feed_override", (PyCFunction)set_feed_override, METH_VARARGS},
    {"set_spindle_override", (PyCFunction)set_spindle_override, METH_VARARGS},
    {"set_feed_hold", (PyCFunction)set_feed_hold, METH_VARARGS},
    {"set_adaptive_feed", (PyCFunction)set_adaptive_feed, METH_VARARGS},
    {"set_digital_output", (PyCFunction)set_digital_output, METH_VARARGS},
    {"set_analog_output", (PyCFunction)set_analog_output, METH_VARARGS},
    {"set_homing_params", (PyCFunction)set_homing_params, METH_VARARGS},
    {"set_axis_max_velocity", (PyCFunction)set_axis_max_velocity, METH_VARARGS},
    {"program_close", (PyCFunction)program_close, METH_NOARGS},
    {"synch_and_export", (PyCFunction)synch_and_export, METH_VARARGS},
    {"clear_recovery_info", (PyCFunction)clear_recovery_info, METH_NOARGS},
    {NULL}
};

static PyTypeObject Command_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "linuxcnc.command",          /*tp_name*/
    sizeof(pyCommandChannel),/*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Command_dealloc,        /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Command_methods,        /*tp_methods*/
    Command_members,        /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)Command_init, /*tp_init*/
    0,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

static int Error_init(pyErrorChannel *self, PyObject *a, PyObject *k) {
    char *file = get_nmlfile();
    if(file == NULL) return -1;

    NML *c = new NML(emcFormat, "emcError", "xemc", file);
    if(!c) {
        PyErr_Format( error, "new NML failed");
        return -1;
    }

    self->c = c;
    return 0;
}

static PyObject* Error_poll(pyErrorChannel *s) {
    if(!s->c->valid()) {
        PyErr_Format( error, "Error buffer invalid" );
        return NULL;
    }
    NMLTYPE type = s->c->read();
    if(type == 0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    PyObject *r = PyTuple_New(2);
    PyTuple_SET_ITEM(r, 0, PyInt_FromLong(type));
#define PASTE(a,b) a ## b
#define TYPECASE_IMPL(tag, type, f) \
    case tag: { \
        std::string msg(((type*)s->c->get_address())->f); \
        PyTuple_SET_ITEM(r, 1, PyString_FromString(msg.c_str())); \
        break; \
    }
#define TYPECASE(x, f) TYPECASE_IMPL(PASTE(x, _TYPE), x, f)
    switch(type) {
        TYPECASE(EMC_OPERATOR_ERROR, text)
        TYPECASE(EMC_OPERATOR_TEXT, text)
        TYPECASE(EMC_OPERATOR_DISPLAY, text)
        TYPECASE(NML_ERROR, error)
        TYPECASE(NML_TEXT, text)
        TYPECASE(NML_DISPLAY, display)
    case EMC_OPERATOR_ERROR_EXTENDED_TYPE: {
        std::string msg(((EMC_OPERATOR_ERROR_EXTENDED*)s->c->get_address())->text);
        // KLUDGE strip context markers from extended error message
        const char *msg_ptr = msg.c_str();
        if (0 == msg.compare(0, 4, CONTEXT_MARKER)) {
            msg_ptr+=strlen(CONTEXT_MARKER);
        }
        PyTuple_SET_ITEM(r, 1, PyString_FromString(msg_ptr));
        break; \
    }
    default:
        {
            char error_string[256];
            sprintf(error_string, "unrecognized error %" PRId32, type);
            PyTuple_SET_ITEM(r, 1, PyString_FromString(error_string));
            break;
        }
    }
    return r;
}

static void Error_dealloc(PyObject *self) {
    delete ((pyErrorChannel*)self)->c;
    PyObject_Del(self);
}

static PyMethodDef Error_methods[] = {
    {"poll", (PyCFunction)Error_poll, METH_NOARGS, "Poll for errors"},
    {NULL}
};

static PyTypeObject Error_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "linuxcnc.error_channel",    /*tp_name*/
    sizeof(pyErrorChannel), /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Error_dealloc,        /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Error_methods,          /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)Error_init,   /*tp_init*/
    0,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

// Assumed to always be non-null
static std::unique_ptr<PreviewKinematicsBase> kinematics(new TrivialKinematics);

static PyObject *pyset_trivial_kinematics(PyObject *s, PyObject *o) {
    const char *geometry="";
    if(!PyArg_ParseTuple(o, "s:set_trivial_kinematics",
                          &geometry
                          ))
        return nullptr;

    kinematics.reset(new TrivialKinematics(geometry));
    Py_RETURN_NONE;
}

static PyObject *pyset_xyz_kinematics(PyObject *, PyObject *) {

    kinematics.reset(new Kinematics_XYZ());

    Py_RETURN_NONE;
}

static PyObject *pyset_xyza_kinematics(PyObject *s, PyObject *o) {
    double y0=0, z0=0;
    int flipped_A=0;
    if(!PyArg_ParseTuple(o, "|ddi:set_xyza_kinematics",
                          &y0,
                          &z0,
                          &flipped_A
                          ))
        return nullptr;

    kinematics.reset(new Kinematics_XYZA(y0, z0, flipped_A));

    Py_RETURN_NONE;
}

#include <GL/gl.h>

static inline void glvertex9(PositionArray const &pt) {
    Array3 p;
    kinematics->forward_position(pt, p);
    glVertex3dv(p.data());
}

#define max3(a,b,c) (std::max((a),std::max((b),(c))))

static void line9(PositionArray const &p1, PositionArray const &p2) {
    if(p1[A] != p2[A] || p1[B] != p2[B] || p1[C] != p2[C]) {
        double dc = max3(
            fabs(p2[A] - p1[A]),
            fabs(p2[B] - p1[B]),
            fabs(p2[C] - p1[C]));
        int st = (int)ceil(max(10., dc/10));
        int i;

        for(i=1; i<=st; i++) {
            double t = i * 1.0 / st;
            double v = 1.0 - t;
            PositionArray pt = p2 * t + p1 * v;
            glvertex9(pt);
        }
    } else {
        glvertex9(p2);
    }
}


static void line9b(PositionArray const &p1, PositionArray const &p2) {
    glvertex9(p1);
    if(p1[A] != p2[A] || p1[B] != p2[B] || p1[C] != p2[C]) {
        double dc = max3(
            fabs(p2[A] - p1[A]),
            fabs(p2[B] - p1[B]),
            fabs(p2[C] - p1[C]));
        int st = (int)ceil(max(10., dc/10));
        int i;

        for(i=1; i<=st; i++) {
            double t = i * 1.0 / st;
            double v = 1.0 - t;
            PositionArray pt = p2 * t + p1 * v;
            glvertex9(pt);
            if(i != st)
                glvertex9(pt);
        }
    } else {
        glvertex9(p2);
    }
}

#if 0
static PyObject *pyline9(PyObject *s, PyObject *o) {
    PositionArray pt1;
    PositionArray pt2;

    if(!PyArg_ParseTuple(o, "(ddddddddd)(ddddddddd):line9",
                          UNPACK_POSITION_ARRAY(pt1),
                          UNPACK_POSITION_ARRAY(pt2)
                          ))
        return NULL;

    line9b(pt1, pt2);

    Py_RETURN_NONE;
}

static PyObject *pyvertex9(PyObject *s, PyObject *o) {
    PositionArray vert;
    Array3 pt;
    if(!PyArg_ParseTuple(o, "(ddddddddd):vertex9",
                          UNPACK_POSITION_ARRAY(vert)))
        return NULL;

    kinematics->forward(vert, pt);
    return Py_BuildValue("(ddd)", &pt[0], &pt[1], &pt[2]);
}
#endif

/**
 * RAII wrapper for glBegin / glEnd to avoid manual management of "first" flag
 */
struct GLBeginEndWrapper
{
private:
    GLBeginEndWrapper(const GLBeginEndWrapper&) = delete;
    GLBeginEndWrapper& operator=(const GLBeginEndWrapper&) = delete;
    int draw_type;

public:
    GLBeginEndWrapper() : draw_type(-1) {}
    bool begin(int draw_type_begin) {
        if (draw_type < 0) {
            draw_type = draw_type_begin;
            glBegin(draw_type);
            return true;
        }
        return false;
    }
    void end() {
        if (draw_type >= 0) {
            glEnd();
            draw_type = -1;
        }
    }
    bool started() const {
        return draw_type >= 0;
    }
    ~GLBeginEndWrapper() {
        end();
    }
};

void draw_lines(std::vector<ToolMotion> const &tool_motions)
{
    PositionArray pl{};
    GLBeginEndWrapper gl_wrapper{};
    for(auto const &motion : tool_motions) {
        if(motion.p_start != pl){
            gl_wrapper.end();
            gl_wrapper.begin(GL_LINE_STRIP);
            glvertex9(motion.p_start);
        }
        line9(motion.p_start, motion.p_end);
        pl = motion.p_end;
    }
}

void draw_lines_for_selection(std::vector<ToolMotion> const &tool_motions)
{
    int active_line_no = -1;
    PositionArray pl{};
    GLBeginEndWrapper gl_wrapper{};
    for(auto const &motion : tool_motions) {
        if(motion.p_start != pl || (motion.line_number != active_line_no)){
            gl_wrapper.end();
            if (motion.line_number != active_line_no) {
                // NOTE: this doesn't currently get passed through, so we only
                // really have 24 bits of resolution in line numbers (16.7
                // million lines ought to be enough for anybody) Effectively
                // "a" is the overflow right now
                unsigned a = (motion.line_number & 0xFF000000) >> 24;
                if (!a) {
                    unsigned r = (motion.line_number & 0x000000FF) >>  0;
                    unsigned g = (motion.line_number & 0x0000FF00) >>  8;
                    unsigned b = (motion.line_number & 0x00FF0000) >> 16;
                    glColor4ub(r,g,b,0xFF);
                } else {
                    // Ignore any lines above our cutoff since we can't distinguish them anyway
                    glColor4ub(0,0,0,0xFF);
                }
                active_line_no = motion.line_number;
            }
            gl_wrapper.begin(GL_LINE_STRIP);
            glvertex9(motion.p_start);
        }
        line9(motion.p_start, motion.p_end);
        pl = motion.p_end;
    }
}

static PyObject *pydraw_lines_by_type(PyObject *s, PyObject *o) {
    int for_selection = 0;

    PyObject *capsule=nullptr;
    PyObject *motion_key_tuple=nullptr;
    if(!PyArg_ParseTuple(o, "O!O!i:draw_lines_by_type",
                &PyCapsule_Type, &capsule, &PyTuple_Type, &motion_key_tuple, &for_selection))
        return NULL;
    if (!capsule) {
        PyErr_Format( error, "Motion data capsule is null!");
        return nullptr;
    }

    char *motion_type_str;
    int exec_mode=-1;
    int tool_no=-1;
    if(!PyArg_ParseTuple(motion_key_tuple, "sii", &motion_type_str, &exec_mode, &tool_no)) {
        PyErr_Format( error, "Invalid motion key tuple");
        return nullptr;
    }
    ToolMotionKey key{motion_type_from_str(motion_type_str), (ParseExecMode)exec_mode, tool_no};

    auto tool_motion_map = static_cast<ToolMotionMap*>(PyCapsule_GetPointer(capsule, "tool_motion_data"));
    if (!tool_motion_map) {
        PyErr_Format( error, "No tool motion map found in capsule");
        return nullptr;
    }

    auto it = tool_motion_map->find(key);
    if (it == tool_motion_map->end()) {
        // No matching tool motion, should be an error but handle this later
        Py_INCREF(Py_None);
        return Py_None;
    }

    auto const &tool_motions = it->second;

    if (for_selection) {
        draw_lines_for_selection(tool_motions);
    } else {
        draw_lines(tool_motions);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *pydraw_dwells(PyObject *s, PyObject *o) {
    PyListObject *li;
    int for_selection = 0, is_lathe = 0, i, n;
    double alpha;
    double delta = 0.015625;

    if(!PyArg_ParseTuple(o, "O!dii:draw_dwells", &PyList_Type, &li, &alpha, &for_selection, &is_lathe))
        return NULL;

    if (for_selection == 0)
        glBegin(GL_LINES);

    for(i=0; i<PyList_GET_SIZE(li); i++) {
        PyObject *it = PyList_GET_ITEM(li, i);
        double red, green, blue, x, y, z;
        int axis;
        if(!PyArg_ParseTuple(it, "i(ddd)dddi", &n, &red, &green, &blue, &x, &y, &z, &axis)) {
            return NULL;
        }
        if (for_selection != 1)
            glColor4d(red, green, blue, alpha);
        if (for_selection == 1) {
            glLoadName(n);
            glBegin(GL_LINES);
        }
        if (is_lathe == 1)
            axis = 1;

        if (axis == 0) {
            glVertex3f(x-delta,y-delta,z);
            glVertex3f(x+delta,y+delta,z);
            glVertex3f(x-delta,y+delta,z);
            glVertex3f(x+delta,y-delta,z);

            glVertex3f(x+delta,y+delta,z);
            glVertex3f(x-delta,y-delta,z);
            glVertex3f(x+delta,y-delta,z);
            glVertex3f(x-delta,y+delta,z);
        } else if (axis == 1) {
            glVertex3f(x-delta,y,z-delta);
            glVertex3f(x+delta,y,z+delta);
            glVertex3f(x-delta,y,z+delta);
            glVertex3f(x+delta,y,z-delta);

            glVertex3f(x+delta,y,z+delta);
            glVertex3f(x-delta,y,z-delta);
            glVertex3f(x+delta,y,z-delta);
            glVertex3f(x-delta,y,z+delta);
        } else {
            glVertex3f(x,y-delta,z-delta);
            glVertex3f(x,y+delta,z+delta);
            glVertex3f(x,y+delta,z-delta);
            glVertex3f(x,y-delta,z+delta);

            glVertex3f(x,y+delta,z+delta);
            glVertex3f(x,y-delta,z-delta);
            glVertex3f(x,y-delta,z+delta);
            glVertex3f(x,y+delta,z-delta);
        }
        if (for_selection == 1)
            glEnd();
    }

    if (for_selection == 0)
        glEnd();

    Py_INCREF(Py_None);
    return Py_None;
}

static PM_CARTESIAN xyz(PositionArray const &p)
{
    return PM_CARTESIAN{p[X], p[Y], p[Z]};
}

static PyObject *pydraw_lines_by_property(PyObject *self, PyObject *args, PyObject *kwargs)
{
    // Assume that linewidth and color are already specified (e.g. for highlighting)
    PyObject *capsule=nullptr;

    int line_number=-1;
    int tool_number=-1;
    // TODO other properties to filter by

    static const char * const kwarg_list[] = {
        "tool_motions_capsule",
        "line_number",
        "tool_number",
        nullptr,
        };

    // KLUDGE shove even more parameters in here...
    if(!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "O!|ii",
            const_cast<char**>(kwarg_list),
            &PyCapsule_Type,
            &capsule,
            &line_number,
            &tool_number)) {
        return nullptr;
    }

    if (!capsule) {
        PyErr_Format( error, "Motion data capsule is null!");
        return nullptr;
    }

    auto tool_motion_map = static_cast<ToolMotionMap*>(PyCapsule_GetPointer(capsule, "tool_motion_data"));
    if (!tool_motion_map) {
        PyErr_Format( error, "No tool motion map found in capsule");
        return nullptr;
    }

    PyObject *coords = PyList_New(0);
    glBegin(GL_LINES);
    for(auto const &motion_set : *tool_motion_map) {
        auto const &key = motion_set.first;
        if (tool_number > 0 && key.tool_pocket_no != tool_number) {
            continue;
        }
        for(auto const &motion : motion_set.second) {
            if (line_number <= 0 || motion.line_number == line_number) {
                line9b(motion.p_start, motion.p_end);
                PM_CARTESIAN center_pt = (xyz(motion.p_end) + xyz(motion.p_start))/2.;
                PyObject *c = PyTuple_New(3);
                for(int i = 0; i < 3; i++) {
                    PyTuple_SET_ITEM(c, i, PyFloat_FromDouble(center_pt[i]));
                }
                PyList_Append(coords, c);
                Py_XDECREF(c);
            }
        }
    }
    glEnd();
    return coords;
}

static PyObject *pydraw_oriented_limit_box(PyObject *self, PyObject *args)
{
    PositionArray cur_pos{};
    // Limit box in tool tip coordinates (i.e. caller subtracts away current tool offset)
    PositionArray min_limits_tool = cur_pos;
    PositionArray max_limits_tool = cur_pos;
    // KLUDGE shove even more parameters in here...
    if(!PyArg_ParseTuple(
            args,
            "(ddd)(ddd)(ddddddddd):draw_oriented_limit_box",
            &min_limits_tool[X],
            &min_limits_tool[Y],
            &min_limits_tool[Z],
            &max_limits_tool[X],
            &max_limits_tool[Y],
            &max_limits_tool[Z],
            UNPACK_POSITION_ARRAY(cur_pos)
            )) {
        return nullptr;
    }
    // Limit box is drawn based on allowed XYZ travel from the current position / orientation
    min_limits_tool = merge(min_limits_tool, cur_pos, AxisMask("111111000"));

    static const AxisMask mask_corners[] = {
        AxisMask("100"),
        AxisMask("000"),
        AxisMask("001"),
        AxisMask("101"),
        AxisMask("001"),
        AxisMask("011"),
        AxisMask("111"),
        AxisMask("011"),
        AxisMask("010"),
        AxisMask("110"),
        AxisMask("010"),
        AxisMask("000"),
        AxisMask("100"),
        AxisMask("101"),
        AxisMask("111"),
        AxisMask("110"),
        AxisMask("100"),
        };

    glBegin(GL_LINE_STRIP);
    for (auto const &m : mask_corners) {
        auto p_corner = merge(min_limits_tool, max_limits_tool, m);
        // Draws rotated limitbox in current geometry
        glvertex9(p_corner);
    }
    glEnd();
    Py_INCREF(Py_None);
    return Py_None;
}

// Logger stuff for online plotting
static std::recursive_mutex logger_mutex;

using RLock = std::lock_guard<std::recursive_mutex>;

struct ColorRGBA {
    unsigned char r, g, b, a;
    bool operator==(const struct ColorRGBA &o) const {
        return std::tie(r, g, b, a) == std::tie(o.r, o.g, o.b, o.a);
    }
    bool operator!=(const struct ColorRGBA &o) const {
        return std::tie(r, g, b, a) != std::tie(o.r, o.g, o.b, o.a);
    }
};

struct LoggerPoint {
    float x, y, z;
    struct ColorRGBA c;
    float rx, ry, rz; // or uvw
    struct ColorRGBA c2;
};

#define NUMCOLORS (6)
#define MAX_POINTS (10000)

// We keep track of the live tool path by recording points every so often in an array and then
// getting opengl to draw the array of points as line vertices.  We build each array of
// points according to the tool number that is active.
// When we notice a tool change has occurred, we mark the tool point list for the previous
// tool as inactive.  That way if that same tool is changed to later on in the program,
// we don't simply append another point to the array and draw a line segment that never
// actually happened.
//
// So the single linked list of tool_point_list structures grows by one with every tool
// change.  New active tool_point_list structures are always added to the head so the
// active ones should be found first.

/**
 * Set of live-plot points (vertices that are connected into a strip of lines).
 * @warning MUST BE A VECTOR OR ARRAY
 * This is used to store points for use in openGL so it must be a contiguous block with a consistent, known stride.
 */
using ToolPoints = std::vector<LoggerPoint>;

/**
 * Stores sets of contiguous tool path sections, organized by tool number.
 */
using ToolPointsMap = std::map<int, std::vector<std::vector<LoggerPoint>>>;

struct pyPositionLogger {
    PyObject_HEAD

    ToolPointsMap logger_tool_points;
    PyObject *visible_tool_list;
    std::set<int> visible_tools;
    int last_tool;

    struct ColorRGBA colors[NUMCOLORS];
    bool exit;
    bool clear;

    int is_xyuv;
    double foam_z;
    double foam_w;


    pyStatChannel *st;

    void retire_active_list();
    ToolPoints & find_active_tool_point_list(int target_tool);
    bool need_draw_tool_points(int tool_num) const;
};

void pyPositionLogger::retire_active_list()
{
    RLock lock(logger_mutex);
    auto v = logger_tool_points[last_tool];
    v.push_back(ToolPoints());
    v.back().reserve(1024);
}

static const double epsilon = 1e-4; // 1-cos(1 deg) ~= 1e-4
static const double tiny = 1e-10; 

static inline bool colinear(float xa, float ya, float za, float xb, float yb, float zb, float xc, float yc, float zc) {
    double dx1 = xa-xb, dx2 = xb-xc;
    double dy1 = ya-yb, dy2 = yb-yc;
    double dz1 = za-zb, dz2 = zb-zc;
    double dp = sqrt(dx1*dx1 + dy1*dy1 + dz1*dz1);
    double dq = sqrt(dx2*dx2 + dy2*dy2 + dz2*dz2);
    if( fabs(dp) < tiny || fabs(dq) < tiny ) return true;
    double dot = (dx1*dx2 + dy1*dy2 + dz1*dz2) / dp / dq;
    if( fabs(1-dot) < epsilon) return true;
    return false;
}

ToolPoints &pyPositionLogger::find_active_tool_point_list(int target_tool) {
    RLock lock(logger_mutex);
    auto &v = logger_tool_points[target_tool];
    if (v.empty()) {
        v.push_back(ToolPoints{});
        v.back().reserve(1024);
    }
    return v.back();
}

static int Logger_init(pyPositionLogger *self, PyObject *a, PyObject *k) {
    struct ColorRGBA *c = self->colors;

    self->last_tool = 0;
    self->exit = false;
    self->clear = false;

    Py_INCREF(Py_None);
    self->visible_tools.clear();

    self->st = 0;
    self->is_xyuv = 0;
    self->foam_z = 0;
    self->foam_w = 1.5;  // temporarily hard-code
    if(!PyArg_ParseTuple(a, "O!(BBBB)(BBBB)(BBBB)(BBBB)(BBBB)(BBBB)|i",
            &Stat_Type, &self->st,
            &c[0].r,&c[0].g, &c[0].b, &c[0].a,
            &c[1].r,&c[1].g, &c[1].b, &c[1].a,
            &c[2].r,&c[2].g, &c[2].b, &c[2].a,
            &c[3].r,&c[3].g, &c[3].b, &c[3].a,
            &c[4].r,&c[4].g, &c[4].b, &c[4].a,
            &c[5].r,&c[5].g, &c[5].b, &c[5].a,
            &self->is_xyuv
            ))
        return -1;
    Py_INCREF(self->st);
    return 0;
}

static void Logger_dealloc(pyPositionLogger *s) {
    Py_XDECREF(s->st);
    PyObject_Del(s);
}

static PyObject *Logger_set_depth(pyPositionLogger *s, PyObject *o) {
    double z, w;
    if(!PyArg_ParseTuple(o, "dd:logger.set_depth", &z, &w)) return NULL;
    s->foam_z = z;
    s->foam_w = w;
    Py_INCREF(Py_None);
    return Py_None;
}

static double dist2(double x1, double y1, double x2, double y2) {
    double dx = x2-x1;
    double dy = y2-y1;
    return dx*dx + dy*dy;
}

static PyObject *Logger_start(pyPositionLogger *s, PyObject *o) {
    double interval;
    struct timespec ts;

    if(!PyArg_ParseTuple(o, "d:logger.start", &interval)) return NULL;
    ts.tv_sec = (int)interval;
    ts.tv_nsec = (long int)(1e9 * (interval - ts.tv_sec));

    Py_INCREF(s->st);

    s->exit = false;
    s->clear = true;

    Py_BEGIN_ALLOW_THREADS
        while (!s->exit) {
        nanosleep(&ts, NULL);
        if (s->clear) {
            // Clear point lists for all tools.  We dealloc them all because there can be a lot of
            // inactive tool point lists and trying to collapse them all together is more work than just
            // starting fresh.

            RLock lock(logger_mutex);
            s->logger_tool_points.clear();
            s->clear = false;
        }

        if (!s->st->c->valid() || !(s->st->c->peek() == EMC_STAT_TYPE)) {
            continue;
        }

        EMC_STAT *status = static_cast<EMC_STAT*>(s->st->c->get_address());

        int colornum = status->motion.traj.motion_type;
        if (colornum < 0 || colornum > NUMCOLORS) colornum = 0;
        struct ColorRGBA c = s->colors[colornum];

        if (status->io.tool.toolInSpindle != s->last_tool) {
            s->retire_active_list();
        }
        ToolPoints &pl = s->find_active_tool_point_list(status->io.tool.toolInSpindle);

        struct LoggerPoint *op = pl.empty() ? nullptr : &(*(pl.rbegin()));
        struct LoggerPoint *oop = pl.size() < 2 ? nullptr : &(*(pl.rbegin()+1));
        bool changed_color = false;
        // WARNING: taking care not to access op or oop if the size isn't at the minimum
        bool add_point = !op || (changed_color = (c != op->c)) || !oop;
        double x, y, z, rx, ry, rz;

        if (s->is_xyuv) {
            x = status->motion.traj.position.tran.x - status->task.toolOffset.tran.x;
            y = status->motion.traj.position.tran.y - status->task.toolOffset.tran.y;
            z = s->foam_z;
            rx = status->motion.traj.position.u - status->task.toolOffset.u;
            ry = status->motion.traj.position.v - status->task.toolOffset.v;
            rz = s->foam_w;
            /* TODO .01, the distance at which a preview line is dropped,
                 * should either be dependent on units or configurable, because
                 * 0.1 is inappropriate for mm systems
                 */
            add_point = add_point || (dist2(x, y, oop->x, oop->y) > .01)
                        || (dist2(rx, ry, oop->rx, oop->ry) > .01);
            add_point = add_point || !colinear(x, y, z,
                                               op->x, op->y, op->z,
                                               oop->x, oop->y, oop->z);
            add_point = add_point || !colinear(rx, ry, rz,
                                               op->rx, op->ry, op->rz,
                                               oop->rx, oop->ry, oop->rz);
        } else {
            PositionArray const &pt = {
                status->motion.traj.position.tran.x - status->task.toolOffset.tran.x,
                status->motion.traj.position.tran.y - status->task.toolOffset.tran.y,
                status->motion.traj.position.tran.z - status->task.toolOffset.tran.z,
                status->motion.traj.position.a - status->task.toolOffset.a,
                status->motion.traj.position.b - status->task.toolOffset.b,
                status->motion.traj.position.c - status->task.toolOffset.c,
                status->motion.traj.position.u - status->task.toolOffset.u,
                status->motion.traj.position.v - status->task.toolOffset.v,
                status->motion.traj.position.w - status->task.toolOffset.w};

                Array3 position_xyz, rotation_xyz;
                kinematics->forward(pt, position_xyz, rotation_xyz);
                x = position_xyz[X]; y = position_xyz[Y]; z = position_xyz[Z];
                rx = rotation_xyz[X]; ry = rotation_xyz[Y]; rz = rotation_xyz[Z];

            add_point = add_point || !colinear(x, y, z,
                                               op->x, op->y, op->z,
                                               oop->x, oop->y, oop->z);
        }

        if (add_point) {
            if (changed_color) {
                // when changing color we need to record an extra point
                RLock lock(logger_mutex);
                pl.push_back({
                    op->x,
                    op->y,
                    op->z,
                    c,
                    (float)rx,
                    (float)ry,
                    (float)rz,
                    c,
                    });
            }
            RLock lock(logger_mutex);
            pl.push_back({
                (float)x,
                (float)y,
                (float)z,
                c,
                (float)rx,
                (float)ry,
                (float)rz,
                c,
                });

        } else {
            // update position of last recorded point so that next delta calc is less
            if (!pl.empty()) {
                struct LoggerPoint &np = pl.back();
                np.x = x; np.y = y; np.z = z;
                np.rx = rx; np.ry = ry; np.rz = rz;
            }
        }

        s->last_tool = status->io.tool.toolInSpindle;
    }
    Py_END_ALLOW_THREADS
        Py_DECREF(s->st);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Logger_clear(pyPositionLogger *s, PyObject *o) {
    s->clear = true;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Logger_stop(pyPositionLogger *s, PyObject *o) {
    s->exit = true;
    Py_INCREF(Py_None);
    return Py_None;
}

bool pyPositionLogger::need_draw_tool_points(int tool_num) const
{
    return visible_tools.empty() || visible_tools.find(tool_num) != visible_tools.end();
}

static PyObject* Logger_draw_live_plot(pyPositionLogger *s, PyObject *o) {
    if (!s->clear) {
        RLock lock(logger_mutex);

        // Give loop optimization a shot
        const bool is_foam = s->is_xyuv;

        for (auto const &it : s->logger_tool_points) {
            if (it.second.empty()) {
                continue;
            }
            auto const &points = it.second.back();
            const int tool_num = it.first;

            // Should we draw this tool's point list or not?
            if (!s->need_draw_tool_points(tool_num)) {
                continue;
            }

            if (is_foam) {
                glVertexPointer(3, GL_FLOAT,
                                sizeof(struct LoggerPoint)/2, &points[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE,
                               sizeof(struct LoggerPoint)/2, &points[0].c);
                glEnableClientState(GL_COLOR_ARRAY);
                glEnableClientState(GL_VERTEX_ARRAY);
                glDrawArrays(GL_LINES, 0, 2*points.size());
            } else {
                glVertexPointer(3, GL_FLOAT,
                                sizeof(struct LoggerPoint), &points[0].x);
                glColorPointer(4, GL_UNSIGNED_BYTE,
                               sizeof(struct LoggerPoint), &points[0].c);
                glEnableClientState(GL_COLOR_ARRAY);
                glEnableClientState(GL_VERTEX_ARRAY);
                glDrawArrays(GL_LINE_STRIP, 0, points.size());
            }
        }  
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *Logger_last(pyPositionLogger *s, PyObject *o) {
    int flag=1;
    if(!PyArg_ParseTuple(o, "|i:emc.positionlogger.last", &flag)) return NULL;
    PyObject *result = NULL;
    RLock lock(logger_mutex);

    ToolPoints &points = s->find_active_tool_point_list(s->last_tool);
    if (points.empty()) {
        Py_INCREF(Py_None);
        result = Py_None;
    } else {
        result = PyTuple_New(6);
        struct LoggerPoint &p = points.back();
        PyTuple_SET_ITEM(result, 0, PyFloat_FromDouble(p.x));
        PyTuple_SET_ITEM(result, 1, PyFloat_FromDouble(p.y));
        PyTuple_SET_ITEM(result, 2, PyFloat_FromDouble(p.z));
        PyTuple_SET_ITEM(result, 3, PyFloat_FromDouble(p.rx));
        PyTuple_SET_ITEM(result, 4, PyFloat_FromDouble(p.ry));
        PyTuple_SET_ITEM(result, 5, PyFloat_FromDouble(p.rz));
    }
    return result;
}

static PyObject *Logger_set_visible_tools(pyPositionLogger *s, PyObject *o) {
    PyObject *li;
    if (!PyArg_ParseTuple(o, "O:set_visible_tools", &li)) {
        return NULL;
    }

    if ((li != Py_None) && (! PyList_Check(li))) {
        PyErr_SetString(PyExc_TypeError, "set_visible_tools takes None or a list of tool numbers");
        return NULL;
    }

    if (li == Py_None) {
        s->visible_tools.clear();
    } else {
        for (Py_ssize_t ix = 0; ix < PyList_Size(li); ix++) {
            s->visible_tools.insert(PyInt_AsLong(PyList_GetItem(li, ix)));
        }
    }

    Py_INCREF(Py_None);
    PyObject *result = Py_None;
    return result;   
}

static PyObject* Logger_get_npts(pyPositionLogger *s, PyObject *o) {
    RLock lock(logger_mutex);
    long total_npts = 0;
    for (auto const &it_tool : s->logger_tool_points) {
        for (auto const &it_pts : it_tool.second) {
            total_npts += it_pts.size();
        }
    }

    return PyLong_FromLong(total_npts);
}

static PyMethodDef Logger_methods[] = {
    {"start", (PyCFunction)Logger_start, METH_VARARGS,
        "Start the position logger and run every ARG seconds"},
    {"clear", (PyCFunction)Logger_clear, METH_NOARGS,
        "Clear the position logger"},
    {"stop", (PyCFunction)Logger_stop, METH_NOARGS,
        "Stop the position logger"},
    {"draw_live_plot", (PyCFunction)Logger_draw_live_plot, METH_NOARGS,
        "Plot the backplot now"},
    {"set_depth", (PyCFunction)Logger_set_depth, METH_VARARGS,
        "set the Z and W depths for foam cutter"},
    {"last", (PyCFunction)Logger_last, METH_VARARGS,
        "Return the most recent point on the plot or None"},
    {"set_visible_tools", (PyCFunction)Logger_set_visible_tools, METH_VARARGS,
        "Set the list of tool numbers the live plot should filter its drawing on.  None means all tools are visible."},
    {"get_npts", (PyCFunction)Logger_get_npts, METH_NOARGS,
        "Get total number of points logged"},
    {NULL, NULL, 0, NULL},
};

static PyTypeObject PositionLoggerType = {
    PyObject_HEAD_INIT(NULL)
    0,                      /*ob_size*/
    "linuxcnc.positionlogger",   /*tp_name*/
    sizeof(pyPositionLogger), /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)Logger_dealloc, /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    0,                      /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    Logger_methods,         /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)Logger_init,  /*tp_init*/
    0,                      /*tp_alloc*/
    PyType_GenericNew,      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

static PyMethodDef emc_methods[] = {
    {"draw_lines_by_type", pydraw_lines_by_type, METH_VARARGS, "Draw a subset of lines by key (using internal storage in gcodemodule for lines)"},
    {"draw_lines_by_property", (PyCFunction)pydraw_lines_by_property, METH_VARARGS | METH_KEYWORDS, "Draw a subset of lines that match the specified property (e.g. tool number, line number)"},
    {"draw_dwells", pydraw_dwells, METH_VARARGS, "Draw a bunch of dwell positions in the 'rs274.glcanon' format"},
    {"draw_oriented_limit_box", pydraw_oriented_limit_box, METH_VARARGS, "Draw the tool-oriented limit box using the currently active preview kinematics"},
//    {"line9", pyline9, METH_VARARGS, "Draw a single line in the 'rs274.glcanon' format; assumes glBegin(GL_LINES)"},
//    {"vertex9", pyvertex9, METH_VARARGS, "Get the 3d location for a 9d point"},
    {"set_preview_trivial_kinematics", pyset_trivial_kinematics, METH_VARARGS, "Preview tool motions with trivial kinematics from the provided geometry specification"},
    {"set_preview_xyz_kinematics", pyset_xyz_kinematics, METH_NOARGS, "Preview tool motions with default XYZ kinematics"},
    {"set_preview_xyza_kinematics", pyset_xyza_kinematics, METH_VARARGS, "Preview tool motions with XYZA kinematics, specifying a fixed A axis intercept in the workspace"},
    {}
};

/* ENUM defines an integer constant with the same name as the C constant.
 * ENUMX defines an integer constant with the first i characters of the C
 * constant name removed (so that ENUMX(4,RCS_TASK_MODE_MDI) creates a constant
 * named TASK_MODE_MDI) */

#define ENUM(e) PyModule_AddIntConstant(m, const_cast<char*>(#e), e)
#define ENUMX(x,e) PyModule_AddIntConstant(m, x + const_cast<char*>(#e), e)

PyMODINIT_FUNC
initlinuxcnc(void) {
    emcInitGlobals();
    verbose_nml_error_messages = 0;
    clear_rcs_print_flag(~0);

    m = Py_InitModule3("linuxcnc", emc_methods, "Interface to LinuxCNC");

    PyType_Ready(&Stat_Type);
    PyType_Ready(&Command_Type);
    PyType_Ready(&Error_Type);
    PyType_Ready(&Ini_Type);
    error = PyErr_NewException((char*)"linuxcnc.error", PyExc_RuntimeError, NULL);

    PyModule_AddObject(m, "stat", (PyObject*)&Stat_Type);
    PyModule_AddObject(m, "command", (PyObject*)&Command_Type);
    PyModule_AddObject(m, "error_channel", (PyObject*)&Error_Type);
    PyModule_AddObject(m, "ini", (PyObject*)&Ini_Type);
    PyModule_AddObject(m, "error", error);

    PyType_Ready(&PositionLoggerType);
    PyModule_AddObject(m, "positionlogger", (PyObject*)&PositionLoggerType);

    PyModule_AddStringConstant(m, "PREFIX", EMC2_HOME);
    PyModule_AddStringConstant(m, "SHARE", EMC2_HOME "/share");
//    PyModule_AddStringConstant(m, "nmlfile", EMC2_DEFAULT_NMLFILE);
    char *envPtr = getenv("NML_FILE");
    // if not in environment use config.h #define
    if (envPtr == NULL) {
        PyModule_AddStringConstant(m, "nmlfile", EMC2_DEFAULT_NMLFILE);
    }
    else {
        PyModule_AddStringConstant(m, "nmlfile", envPtr);
    }

    PyModule_AddIntConstant(m, "OPERATOR_ERROR", EMC_OPERATOR_ERROR_TYPE);
    PyModule_AddIntConstant(m, "OPERATOR_TEXT", EMC_OPERATOR_TEXT_TYPE);
    PyModule_AddIntConstant(m, "OPERATOR_DISPLAY", EMC_OPERATOR_DISPLAY_TYPE);
    PyModule_AddIntConstant(m, "NML_ERROR", NML_ERROR_TYPE);
    PyModule_AddIntConstant(m, "NML_TEXT", NML_TEXT_TYPE);
    PyModule_AddIntConstant(m, "NML_DISPLAY", NML_DISPLAY_TYPE);

    PyStructSequence_InitType(&ToolResultType, &tool_result_desc);
    PyModule_AddObject(m, "tool", (PyObject*)&ToolResultType);
    PyModule_AddObject(m, "version", PyString_FromString(PACKAGE_VERSION));
    PyModule_AddObject(m, "context_marker", PyString_FromString(CONTEXT_MARKER));

    ENUMX(4, EMC_AXIS_LINEAR);
    ENUMX(4, EMC_AXIS_ANGULAR);

    ENUMX(9, EMC_TASK_INTERP_IDLE);
    ENUMX(9, EMC_TASK_INTERP_READING);
    ENUMX(9, EMC_TASK_INTERP_PAUSED);
    ENUMX(9, EMC_TASK_INTERP_WAITING);
    ENUMX(9, EMC_TASK_INTERP_SEEKING);

    ENUMX(9, EMC_TASK_MODE_MDI);
    ENUMX(9, EMC_TASK_MODE_MANUAL);
    ENUMX(9, EMC_TASK_MODE_AUTO);

    ENUMX(9, EMC_TASK_STATE_OFF);
    ENUMX(9, EMC_TASK_STATE_ON);
    ENUMX(9, EMC_TASK_STATE_ESTOP);
    ENUMX(9, EMC_TASK_STATE_ESTOP_RESET);

    ENUMX(6, LOCAL_SPINDLE_FORWARD);
    ENUMX(6, LOCAL_SPINDLE_REVERSE);
    ENUMX(6, LOCAL_SPINDLE_OFF);
    ENUMX(6, LOCAL_SPINDLE_INCREASE);
    ENUMX(6, LOCAL_SPINDLE_DECREASE);
    ENUMX(6, LOCAL_SPINDLE_CONSTANT);

    ENUMX(6, CANON_UNITS_INCHES);
    ENUMX(6, CANON_UNITS_MM);

    ENUMX(6, LOCAL_MIST_ON);
    ENUMX(6, LOCAL_MIST_OFF);

    ENUMX(6, LOCAL_FLOOD_ON);
    ENUMX(6, LOCAL_FLOOD_OFF);

    ENUMX(6, LOCAL_BRAKE_ENGAGE);
    ENUMX(6, LOCAL_BRAKE_RELEASE);

    ENUMX(6, LOCAL_JOG_STOP);
    ENUMX(6, LOCAL_JOG_CONTINUOUS);
    ENUMX(6, LOCAL_JOG_INCREMENT);

    ENUMX(6, LOCAL_AUTO_RUN);
    ENUMX(6, LOCAL_AUTO_PAUSE);
    ENUMX(6, LOCAL_AUTO_RESUME);
    ENUMX(6, LOCAL_AUTO_STEP);
    ENUMX(6, LOCAL_AUTO_REVERSE);
    ENUMX(6, LOCAL_AUTO_FORWARD);

    ENUMX(4, EMC_TRAJ_MODE_FREE);
    ENUMX(4, EMC_TRAJ_MODE_COORD);
    ENUMX(4, EMC_TRAJ_MODE_TELEOP);

    ENUM(KINEMATICS_IDENTITY);
    ENUM(KINEMATICS_FORWARD_ONLY);
    ENUM(KINEMATICS_INVERSE_ONLY);
    ENUM(KINEMATICS_BOTH);

    ENUMX(4, EMC_DEBUG_CONFIG);
    ENUMX(4, EMC_DEBUG_VERSIONS);
    ENUMX(4, EMC_DEBUG_TASK_ISSUE);
    ENUMX(4, EMC_DEBUG_NML);
    ENUMX(4, EMC_DEBUG_MOTION_TIME);
    ENUMX(4, EMC_DEBUG_INTERP);
    ENUMX(4, EMC_DEBUG_RCS);
    ENUMX(4, EMC_DEBUG_INTERP_LIST);

    ENUMX(9, EMC_TASK_EXEC_ERROR);
    ENUMX(9, EMC_TASK_EXEC_DONE);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_MOTION);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_MOTION_QUEUE);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_IO);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_MOTION_AND_IO);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_DELAY);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_SYSTEM_CMD);
    ENUMX(9, EMC_TASK_EXEC_WAITING_FOR_SPINDLE_ORIENTED);

    // Constants for indexing into the status.gcodes[] array
    ENUMX(0, G_CODE_SEQUENCE_NUMBER);
    ENUMX(0, G_CODE_MOTION_MODE);
    ENUMX(0, G_CODE_MODE_0);
    ENUMX(0, G_CODE_PLANE);
    ENUMX(0, G_CODE_CUTTER_SIDE);
    ENUMX(0, G_CODE_UNITS);
    ENUMX(0, G_CODE_DISTANCE_MODE);
    ENUMX(0, G_CODE_FEED_MODE);
    ENUMX(0, G_CODE_ORIGIN);
    ENUMX(0, G_CODE_TOOL_LENGTH_OFFSET);
    ENUMX(0, G_CODE_RETRACT_MODE);
    ENUMX(0, G_CODE_PATH_MODE);
    // Note there is a gap here on purpose.  G_CODE_unsupported is a valid enum
    // but makes no sense to expose to the python side yet until its actually
    // implemented on the linuxcnc side.
    ENUMX(0, G_CODE_SPINDLE_MODE);
    ENUMX(0, G_CODE_DISTANCE_MODE_IJK);
    ENUMX(0, G_CODE_LATHE_DIAMETER_MODE);

    ENUM(PREP_NONE);
    ENUM(PREP_LINEAR_LEADIN);
    ENUM(PREP_PLUNGE_LEADIN);

    // Constants for indexing into the status.mcodes[] array
    ENUMX(0, M_CODE_SEQUENCE_NUMBER);
    ENUMX(0, M_CODE_STOPPING);
    ENUMX(0, M_CODE_SPINDLE);
    ENUMX(0, M_CODE_TOOLCHANGE);
    ENUMX(0, M_CODE_MIST);
    ENUMX(0, M_CODE_FLOOD);
    ENUMX(0, M_CODE_OVERRIDES);
    ENUMX(0, M_CODE_ADAPTIVE_FEED);
    ENUMX(0, M_CODE_FEED_HOLD);

    ENUM(RCS_DONE);
    ENUM(RCS_EXEC);
    ENUM(RCS_ERROR);
}


// # vim:sw=4:sts=4:et:ts=8:


