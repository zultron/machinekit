#include "mock_motion_global.h"
#include "mot_priv.h"
#include "motion_struct.h"
#include "stdlib.h"

int DEBUG_MOTION = 0;
int num_joints = EMCMOT_MAX_JOINTS;	/* default number of joints present */
int num_dio = 4;			/* default number of motion synched DIO */
int num_aio = 4;			/* default number of motion synched AIO */

static const emcmot_struct_t EMPTY_SHM;

static struct emcmot_struct_t s;
struct emcmot_struct_t *emcmotStruct = NULL;
/* ptrs to either buffered copies or direct memory for
   command and status */
struct emcmot_command_t *emcmotCommand = NULL;
struct emcmot_status_t *emcmotStatus = NULL;
struct emcmot_config_t *emcmotConfig = NULL;
struct emcmot_debug_t *emcmotDebug = NULL;
struct emcmot_internal_t *emcmotInternal = NULL;
struct emcmot_error_t *emcmotError = NULL;	/* unused for RT_FIFO */

struct emcmot_joint_t *joints = NULL;

void init_motion_globals(double a_max)
{
    s = EMPTY_SHM;
    emcmotStruct = &s;
    emcmotConfig = &emcmotStruct->config;
    emcmotCommand = &emcmotStruct->command;
    emcmotDebug = &emcmotStruct->debug;
    emcmotError = &emcmotStruct->error;
    emcmotInternal = &emcmotStruct->internal;
    emcmotStatus = &emcmotStruct->status;

    emcmotConfig->arc_blend_cfg.enable = 1;
    emcmotConfig->arc_blend_cfg.allow_fallback = 1;
    emcmotConfig->arc_blend_cfg.optimization_depth = 100;
    emcmotConfig->arc_blend_cfg.gap_cycles = 5;
    emcmotConfig->arc_blend_cfg.ramp_frequency = 100;
    emcmotConfig->maxFeedScale = 2.0;

    emcmotConfig->min_displacement_cfg.min_linear_displacement = 1e-7;
    emcmotConfig->min_displacement_cfg.min_angular_displacement = 1e-7;

    joints = emcmotDebug->joints;

    // assume an XYZ machine
    emcmotConfig->numJoints=9;
    emcmot_joint_t mock_xyz={0};
    mock_xyz.max_pos_limit = 20;
    mock_xyz.min_pos_limit = -20;
    mock_xyz.vel_limit = 2.0;
    mock_xyz.acc_limit = a_max;

    for (int i = 0; i < 3; ++i) {
        joints[i] = mock_xyz;
    }

    emcmot_joint_t mock_abc={0};
    mock_abc.max_pos_limit = 1000;
    mock_abc.min_pos_limit = -1000;
    mock_abc.vel_limit = 20.0;
    mock_abc.acc_limit = 3.0*a_max;

    for (int i = 3; i < 6; ++i) {
        joints[i] = mock_abc;
    }

    for (int i = 6; i < 9; ++i) {
        joints[i] = mock_xyz;
    }

    emcmotConfig->biarc_solver_cfg.max_iterations = 15;
    emcmotConfig->biarc_solver_cfg.velocity_cutoff = 0.001;
    emcmotConfig->biarc_solver_cfg.convergence_tolerance = 0.000001;
    emcmotConfig->biarc_solver_cfg.velocity_abs_tolerance = 0.0001;
    emcmotConfig->biarc_solver_cfg.velocity_rel_tolerance = 0.02;
    emcmotConfig->biarc_solver_cfg.deviation_abs_tolerance = 0.00001;
    emcmotConfig->biarc_solver_cfg.feed_override_allowance = 1.0;
}
