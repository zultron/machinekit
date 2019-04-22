/********************************************************************
* Description: emc.hh
*   Declarations for EMC NML vocabulary
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/
#ifndef EMC_TYPES_HH
#define EMC_TYPES_HH

#include "config.h"
#include "emcglb.h"		// EMC_AXIS_MAX
#include "nml_type.hh"
#include "motion_types.h"

// Forward class declarations
class EMC_AXIS_STAT;
class EMC_TRAJ_STAT;
class EMC_MOTION_STAT;
class EMC_TASK_STAT;
class EMC_TOOL_STAT;
class EMC_AUX_STAT;
class EMC_SPINDLE_STAT;
class EMC_COOLANT_STAT;
class EMC_LUBE_STAT;
class EMC_IO_STAT;
class EMC_STAT;
class CMS;
class RCS_CMD_CHANNEL;
class RCS_STAT_CHANNEL;
class NML;
struct EmcPose;
struct PM_CARTESIAN;

// ---------------------
// EMC TYPE DECLARATIONS
// ---------------------

// NML for base EMC

enum EMCNMLMessageTypes {
    EMC_OPERATOR_ERROR_TYPE = 11,
    EMC_OPERATOR_TEXT_TYPE = 12,
    EMC_OPERATOR_DISPLAY_TYPE = 13,

    EMC_NULL_TYPE = 21,

    EMC_SET_DEBUG_TYPE = 22,

    EMC_SYSTEM_CMD_TYPE = 30,

    // NML for EMC_AXIS

    EMC_AXIS_SET_AXIS_TYPE = 101,
    EMC_AXIS_SET_UNITS_TYPE = 102,
    /* gap because of deleted message types */


    EMC_AXIS_SET_MIN_POSITION_LIMIT_TYPE = 107,
    EMC_AXIS_SET_MAX_POSITION_LIMIT_TYPE = 108,
    EMC_AXIS_SET_FERROR_TYPE = 111,
    EMC_AXIS_SET_HOMING_PARAMS_TYPE = 112,
    // gap because of deleted message types

    EMC_AXIS_SET_MIN_FERROR_TYPE = 115,
    EMC_AXIS_SET_MAX_VELOCITY_TYPE = 116,
    // gap because of deleted message types

    EMC_AXIS_INIT_TYPE = 118,
    EMC_AXIS_HALT_TYPE = 119,
    EMC_AXIS_ABORT_TYPE = 120,
    EMC_AXIS_ENABLE_TYPE = 121,
    EMC_AXIS_DISABLE_TYPE = 122,
    EMC_AXIS_HOME_TYPE = 123,
    EMC_AXIS_UNHOME_TYPE = 135,
    EMC_AXIS_JOG_TYPE = 124,
    EMC_AXIS_INCR_JOG_TYPE = 125,
    EMC_AXIS_ABS_JOG_TYPE = 126,
    EMC_AXIS_ACTIVATE_TYPE = 127,
    EMC_AXIS_DEACTIVATE_TYPE = 128,
    EMC_AXIS_OVERRIDE_LIMITS_TYPE = 129,
    EMC_AXIS_LOAD_COMP_TYPE = 131,
    // gap because of deleted message type (EMC_AXIS_ALTER_TYPE),
    EMC_AXIS_SET_BACKLASH_TYPE = 134,

    EMC_AXIS_STAT_TYPE = 199,

    // NML for EMC_TRAJ

    EMC_TRAJ_SET_AXES_TYPE = 201,
    EMC_TRAJ_SET_UNITS_TYPE = 202,
    EMC_TRAJ_SET_CYCLE_TIME_TYPE = 203,
    EMC_TRAJ_SET_MODE_TYPE = 204,
    EMC_TRAJ_SET_VELOCITY_TYPE = 205,
    EMC_TRAJ_SET_ACCELERATION_TYPE = 206,
    EMC_TRAJ_SET_MAX_VELOCITY_TYPE = 207,
    EMC_TRAJ_SET_MAX_ACCELERATION_TYPE = 208,
    EMC_TRAJ_SET_SCALE_TYPE = 209,
    EMC_TRAJ_SET_RAPID_SCALE_TYPE = 238,
    EMC_TRAJ_SET_MOTION_ID_TYPE = 210,

    EMC_TRAJ_INIT_TYPE = 211,
    EMC_TRAJ_HALT_TYPE = 212,
    EMC_TRAJ_ENABLE_TYPE = 213,
    EMC_TRAJ_DISABLE_TYPE = 214,
    EMC_TRAJ_ABORT_TYPE = 215,
    EMC_TRAJ_PAUSE_TYPE = 216,
    EMC_TRAJ_STEP_TYPE = 217,
    EMC_TRAJ_RESUME_TYPE = 218,
    EMC_TRAJ_DELAY_TYPE = 219,
    EMC_TRAJ_LINEAR_MOVE_TYPE = 220,
    EMC_TRAJ_CIRCULAR_MOVE_TYPE = 221,
    EMC_TRAJ_SET_TERM_COND_TYPE = 222,
    EMC_TRAJ_SET_OFFSET_TYPE = 223,
    EMC_TRAJ_SET_G5X_TYPE = 224,
    EMC_TRAJ_SET_HOME_TYPE = 225,
    EMC_TRAJ_SET_ROTATION_TYPE = 226,
    EMC_TRAJ_SET_G92_TYPE = 227,
    /* gap because of removed messages */

    EMC_TRAJ_CLEAR_PROBE_TRIPPED_FLAG_TYPE = 228,
    EMC_TRAJ_PROBE_TYPE = 229,
    EMC_TRAJ_SET_TELEOP_ENABLE_TYPE = 230,
    EMC_TRAJ_SET_TELEOP_VECTOR_TYPE = 231,
    EMC_TRAJ_SET_SPINDLESYNC_TYPE = 232,
    EMC_TRAJ_SET_SPINDLE_SCALE_TYPE = 233,
    EMC_TRAJ_SET_FO_ENABLE_TYPE = 234,
    EMC_TRAJ_SET_SO_ENABLE_TYPE = 235,
    EMC_TRAJ_SET_FH_ENABLE_TYPE = 236,
    EMC_TRAJ_RIGID_TAP_TYPE = 237,

    EMC_TRAJ_STAT_TYPE = 299,

    // EMC_MOTION aggregate class type declaration

    EMC_MOTION_INIT_TYPE = 301,
    EMC_MOTION_HALT_TYPE = 302,
    EMC_MOTION_ABORT_TYPE = 303,
    EMC_MOTION_SET_AOUT_TYPE = 304,
    EMC_MOTION_SET_DOUT_TYPE = 305,
    EMC_MOTION_ADAPTIVE_TYPE = 306,

    EMC_MOTION_STAT_TYPE = 399,

    // NML for EMC_TASK

    EMC_TASK_INIT_TYPE = 501,
    EMC_TASK_HALT_TYPE = 502,
    EMC_TASK_ABORT_TYPE = 503,
    EMC_TASK_SET_MODE_TYPE = 504,
    EMC_TASK_SET_STATE_TYPE = 505,
    EMC_TASK_PLAN_OPEN_TYPE = 506,
    EMC_TASK_PLAN_RUN_TYPE = 507,
    EMC_TASK_PLAN_READ_TYPE = 508,
    EMC_TASK_PLAN_EXECUTE_TYPE = 509,
    EMC_TASK_PLAN_PAUSE_TYPE = 510,
    EMC_TASK_PLAN_STEP_TYPE = 511,
    EMC_TASK_PLAN_RESUME_TYPE = 512,
    EMC_TASK_PLAN_END_TYPE = 513,
    EMC_TASK_PLAN_CLOSE_TYPE = 514,
    EMC_TASK_PLAN_INIT_TYPE = 515,
    EMC_TASK_PLAN_SYNCH_TYPE = 516,
    EMC_TASK_PLAN_SET_OPTIONAL_STOP_TYPE = 517,
    EMC_TASK_PLAN_SET_BLOCK_DELETE_TYPE = 518,
    EMC_TASK_PLAN_OPTIONAL_STOP_TYPE = 519,

    EMC_TASK_STAT_TYPE = 599,

    // EMC_TOOL type declarations

    EMC_TOOL_INIT_TYPE = 1101,
    EMC_TOOL_HALT_TYPE = 1102,
    EMC_TOOL_ABORT_TYPE = 1103,
    EMC_TOOL_PREPARE_TYPE = 1104,
    EMC_TOOL_LOAD_TYPE = 1105,
    EMC_TOOL_UNLOAD_TYPE = 1106,
    EMC_TOOL_LOAD_TOOL_TABLE_TYPE = 1107,
    EMC_TOOL_SET_OFFSET_TYPE = 1108,
    EMC_TOOL_SET_NUMBER_TYPE = 1109,
    // the following message is sent to io at the very start of an M6
    // even before emccanon issues the move to toolchange position
    EMC_TOOL_START_CHANGE_TYPE = 1110,

    EMC_EXEC_PLUGIN_CALL_TYPE = 1112,
    EMC_IO_PLUGIN_CALL_TYPE = 1113,

    EMC_TASK_SET_TAG_TYPE = 1114,

    EMC_TOOL_STAT_TYPE = 1199,

    // EMC_AUX type declarations

    /* removed #define EMC_AUX_INIT_TYPE = 1201 */
    /* removed #define EMC_AUX_HALT_TYPE = 1202 */
    /* removed #define EMC_AUX_ABORT_TYPE = 1203 */
    /* removed #define EMC_AUX_DIO_WRITE_TYPE = 1204 */
    /* removed #define EMC_AUX_AIO_WRITE_TYPE = 1205 */
    EMC_AUX_ESTOP_ON_TYPE = 1206,
    EMC_AUX_ESTOP_OFF_TYPE = 1207,
    EMC_AUX_ESTOP_RESET_TYPE = 1208,
    EMC_AUX_INPUT_WAIT_TYPE = 1209,

    EMC_AUX_STAT_TYPE = 1299,

    // EMC_SPINDLE type declarations

    /* removed #define EMC_SPINDLE_INIT_TYPE = 1301 */
    /* removed #define EMC_SPINDLE_HALT_TYPE = 1302 */
    /* removed #define EMC_SPINDLE_ABORT_TYPE = 1303 */
    EMC_SPINDLE_ON_TYPE = 1304,
    EMC_SPINDLE_OFF_TYPE = 1305,
    /* removed #define EMC_SPINDLE_FORWARD_TYPE = 1306 */
    /* removed #define EMC_SPINDLE_REVERSE_TYPE = 1307 */
    /* removed #define EMC_SPINDLE_STOP_TYPE = 1308 */
    EMC_SPINDLE_INCREASE_TYPE = 1309,
    EMC_SPINDLE_DECREASE_TYPE = 1310,
    EMC_SPINDLE_CONSTANT_TYPE = 1311,
    EMC_SPINDLE_BRAKE_RELEASE_TYPE = 1312,
    EMC_SPINDLE_BRAKE_ENGAGE_TYPE = 1313,
    /* removed #define EMC_SPINDLE_ENABLE_TYPE = 1314 */
    /* removed #define EMC_SPINDLE_DISABLE_TYPE = 1315 */
    EMC_SPINDLE_SPEED_TYPE = 1316,
    EMC_SPINDLE_ORIENT_TYPE = 1317,
    EMC_SPINDLE_WAIT_ORIENT_COMPLETE_TYPE = 1318,

    EMC_SPINDLE_STAT_TYPE = 1399,

    // EMC_COOLANT type declarations

    /* removed #define EMC_COOLANT_INIT_TYPE = 1401 */
    /* removed #define EMC_COOLANT_HALT_TYPE = 1402 */
    /* removed #define EMC_COOLANT_ABORT_TYPE = 1403 */
    EMC_COOLANT_MIST_ON_TYPE = 1404,
    EMC_COOLANT_MIST_OFF_TYPE = 1405,
    EMC_COOLANT_FLOOD_ON_TYPE = 1406,
    EMC_COOLANT_FLOOD_OFF_TYPE = 1407,

    EMC_COOLANT_STAT_TYPE = 1499,

    // EMC_LUBE type declarations

    /* removed #define EMC_LUBE_INIT_TYPE = 1501 */
    /* removed #define EMC_LUBE_HALT_TYPE = 1502 */
    /* removed #define EMC_LUBE_ABORT_TYPE = 1503 */
    EMC_LUBE_ON_TYPE = 1504,
    EMC_LUBE_OFF_TYPE = 1505,

    EMC_LUBE_STAT_TYPE = 1599,

    // EMC IO point configuration declarations

    /* removed #define EMC_SET_DIO_INDEX_TYPE = 5001 */
    /* removed #define EMC_SET_AIO_INDEX_TYPE = 5002 */

    // EMC_IO aggregate class type declaration

    EMC_IO_INIT_TYPE = 1601,
    EMC_IO_HALT_TYPE = 1602,
    EMC_IO_ABORT_TYPE = 1603,
    EMC_IO_SET_CYCLE_TIME_TYPE = 1604,

    EMC_IO_STAT_TYPE = 1699,

    // EMC aggregate class type declaration

    // these are placeholders
    EMC_LOG_TYPE_IO_CMD = 21,	// command into EMC IO controller
    EMC_LOG_TYPE_TASK_CMD = 51,	// command into EMC Task controller

    EMC_INIT_TYPE = 1901,
    EMC_HALT_TYPE = 1902,
    EMC_ABORT_TYPE = 1903,

    EMC_STAT_TYPE = 1999,
};

enum EMC_TRAJ_TERM_COND {
    EMC_TRAJ_TERM_COND_STOP = 0,
    EMC_TRAJ_TERM_COND_EXACT = 1,
    EMC_TRAJ_TERM_COND_BLEND = 2,
};

// types for EMC_TASK mode
enum EMC_TASK_MODE_ENUM {
    EMC_TASK_MODE_MANUAL = 1,
    EMC_TASK_MODE_AUTO = 2,
    EMC_TASK_MODE_MDI = 3
};

// types for EMC_TASK state
enum EMC_TASK_STATE_ENUM {
    EMC_TASK_STATE_ESTOP = 1,
    EMC_TASK_STATE_ESTOP_RESET = 2,
    EMC_TASK_STATE_OFF = 3,
    EMC_TASK_STATE_ON = 4
};

// types for EMC_TASK execState
enum EMC_TASK_EXEC_ENUM {
    EMC_TASK_EXEC_ERROR = 1,
    EMC_TASK_EXEC_DONE = 2,
    EMC_TASK_EXEC_WAITING_FOR_MOTION = 3,
    EMC_TASK_EXEC_WAITING_FOR_MOTION_QUEUE = 4,
    EMC_TASK_EXEC_WAITING_FOR_IO = 5,
    EMC_TASK_EXEC_WAITING_FOR_MOTION_AND_IO = 7,
    EMC_TASK_EXEC_WAITING_FOR_DELAY = 8,
    EMC_TASK_EXEC_WAITING_FOR_SYSTEM_CMD = 9,
    EMC_TASK_EXEC_WAITING_FOR_SPINDLE_ORIENTED = 10
};

// types for EMC_TASK interpState
enum EMC_TASK_INTERP_ENUM {
    EMC_TASK_INTERP_IDLE = 1,
    EMC_TASK_INTERP_READING = 2,
    EMC_TASK_INTERP_PAUSED = 3,
    EMC_TASK_INTERP_WAITING = 4
};

// types for motion control
enum EMC_TRAJ_MODE_ENUM {
    EMC_TRAJ_MODE_FREE = 1,	// independent-axis motion,
    EMC_TRAJ_MODE_COORD = 2,	// coordinated-axis motion,
    EMC_TRAJ_MODE_TELEOP = 3	// velocity based world coordinates motion,
};

// types for emcIoAbort() reasons
enum EMC_IO_ABORT_REASON_ENUM {
	EMC_ABORT_TASK_EXEC_ERROR = 1,
	EMC_ABORT_AUX_ESTOP = 2,
	EMC_ABORT_MOTION_OR_IO_RCS_ERROR = 3,
	EMC_ABORT_TASK_STATE_OFF = 4,
	EMC_ABORT_TASK_STATE_ESTOP_RESET = 5,
	EMC_ABORT_TASK_STATE_ESTOP = 6,
	EMC_ABORT_TASK_STATE_NOT_ON = 7,
	EMC_ABORT_TASK_ABORT = 8,
	EMC_ABORT_INTERPRETER_ERROR = 9,	// interpreter failed during readahead
	EMC_ABORT_INTERPRETER_ERROR_MDI = 10,	// interpreter failed during MDI execution
	EMC_ABORT_USER = 100  // user-defined abort codes start here
};

// values for EMC_AXIS_SET_AXIS, axisType
enum EmcAxisType {
    EMC_AXIS_LINEAR             = 1,
    EMC_AXIS_ANGULAR            = 2,
};

/**
 * Set the units conversion factor.
 * @see EMC_AXIS_SET_INPUT_SCALE
 */
typedef double                  EmcLinearUnits;
typedef double                  EmcAngularUnits;

#endif				// #ifndef EMC_TYPES_HH
