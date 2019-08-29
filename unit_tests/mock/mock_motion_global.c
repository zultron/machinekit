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

    emcmotConfig->arcBlendEnable = 1;
    emcmotConfig->arcBlendOptDepth = 100;
    emcmotConfig->arcBlendGapCycles = 5;
    emcmotConfig->arcBlendRampFreq = 100;
    emcmotConfig->maxFeedScale = 2.0;

    joints = emcmotDebug->joints;

    // assume an XYZ machine
    emcmotConfig->numJoints=4;
    emcmot_joint_t mock_j={0};
    mock_j.max_pos_limit = 20;
    mock_j.min_pos_limit = -20;
    mock_j.vel_limit = 2.0;
    mock_j.acc_limit = a_max;

    for (int i = 0; i < 4; ++i) {
        joints[i] = mock_j;
    }
}
