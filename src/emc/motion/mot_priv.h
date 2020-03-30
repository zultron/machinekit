/********************************************************************
* Description: mot_priv.h
*   Macros and declarations local to the realtime sources.
*
* Author: 
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
*
********************************************************************/
#ifndef MOT_PRIV_H
#define MOT_PRIV_H

#include "motion_hal.h"
#include "kinematics.h"
#include "motion_shared.h"

struct emcmot_joint_t;
/***********************************************************************
*                   GLOBAL VARIABLE DECLARATIONS                       *
************************************************************************/

/* HAL component ID for motion module */
extern int mot_comp_id;

/* userdefined number of joints. default is EMCMOT_MAX_JOINTS(=8), 
   but can be altered at motmod insmod time */
extern int num_joints;

/* userdefined number of digital IO. default is 4. (EMCMOT_MAX_DIO=64), 
   but can be altered at motmod insmod time */
extern int num_dio;

/* userdefined number of analog IO. default is 4. (EMCMOT_MAX_AIO=16), 
   but can be altered at motmod insmod time */
extern int num_aio;

/* pointer to emcmot_hal_data_t struct in HAL shmem, with all HAL data */
extern emcmot_hal_data_t *emcmot_hal_data;

/* pointer to array of joint structs with all joint data */
/* the actual array may be in shared memory or in kernel space, as
   determined by the init code in motion.c
*/
extern struct emcmot_joint_t *joints;

/* Variable defs */
extern int kinType;
extern int rehomeAll;
extern int DEBUG_MOTION;
extern int EMCMOT_NO_FORWARD_KINEMATICS;
extern KINEMATICS_FORWARD_FLAGS fflags;
extern KINEMATICS_INVERSE_FLAGS iflags;
/* these variables have the servo cycle time and 1/cycle time */
extern double servo_period;
extern double servo_freq;

/***********************************************************************
*                    PUBLIC FUNCTION PROTOTYPES                        *
************************************************************************/

/* function definitions */
extern void emcmotCommandHandler(void *arg, long period);
extern void emcmotController(void *arg, long period);
extern void emcmotSetCycleTime(unsigned long nsec);

/* these are related to synchronized I/O */
extern void emcmotDioWrite(int index, char value);
extern void emcmotAioWrite(int index, double value);

extern void emcmotSetRotaryUnlock(int axis, int unlock);
extern int emcmotGetRotaryIsUnlocked(int axis);

/* homing is no longer in control.c, make functions public */
extern void do_homing_sequence(void);
extern void do_homing(void);

/* loops through the active joints and checks if any are not homed */
extern int checkAllHomed(void);
/* recalculates jog limits */
extern void refresh_jog_limits(struct emcmot_joint_t *joint);
/* handles 'homed' flags, see command.c for details */
extern void clearHomes(int joint_num);

extern void emcmot_config_change(void);
extern void reportError(const char *fmt, ...) __attribute((format(printf,1,2))); /* Use the rtapi_print call */

 /* rtapi_get_time() returns a nanosecond value. In time, we should use a u64
    value for all calcs and only do the conversion to seconds when it is
    really needed. */
#define etime() (((double) rtapi_get_time()) / 1.0e9)

/* macros for reading, writing bit flags */

/* motion flags */

#define GET_MOTION_ERROR_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_ERROR_BIT ? 1 : 0)

#define SET_MOTION_ERROR_FLAG(fl) do { \
    if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_ERROR_BIT; \
    else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_ERROR_BIT; \
} while (0)

#define GET_MOTION_COORD_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_COORD_BIT ? 1 : 0)

#define SET_MOTION_COORD_FLAG(fl) do { \
    if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_COORD_BIT; \
    else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_COORD_BIT; \
} while (0)

#define GET_MOTION_TELEOP_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_TELEOP_BIT ? 1 : 0)

#define SET_MOTION_TELEOP_FLAG(fl) do { \
    if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_TELEOP_BIT; \
    else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_TELEOP_BIT; \
} while (0)

#define GET_MOTION_INPOS_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_INPOS_BIT ? 1 : 0)

#define SET_MOTION_INPOS_FLAG(fl) do { \
    if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_INPOS_BIT; \
    else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_INPOS_BIT; \
} while (0)

#define GET_MOTION_ENABLE_FLAG() (emcmotStatus->motionFlag & EMCMOT_MOTION_ENABLE_BIT ? 1 : 0)

#define SET_MOTION_ENABLE_FLAG(fl) do { \
    if (fl) emcmotStatus->motionFlag |= EMCMOT_MOTION_ENABLE_BIT; \
    else emcmotStatus->motionFlag &= ~EMCMOT_MOTION_ENABLE_BIT; \
} while (0)

/* joint flags */

#define GET_JOINT_ENABLE_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_ENABLE_BIT ? 1 : 0)

#define SET_JOINT_ENABLE_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_ENABLE_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_ENABLE_BIT; \
} while (0)

#define GET_JOINT_ACTIVE_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_ACTIVE_BIT ? 1 : 0)

#define SET_JOINT_ACTIVE_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_ACTIVE_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_ACTIVE_BIT; \
} while (0)

#define GET_JOINT_INPOS_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_INPOS_BIT ? 1 : 0)

#define SET_JOINT_INPOS_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_INPOS_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_INPOS_BIT; \
} while (0)

#define GET_JOINT_ERROR_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_ERROR_BIT ? 1 : 0)

#define SET_JOINT_ERROR_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_ERROR_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_ERROR_BIT; \
} while (0)

#define GET_JOINT_PHL_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_MAX_HARD_LIMIT_BIT ? 1 : 0)

#define SET_JOINT_PHL_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_MAX_HARD_LIMIT_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_MAX_HARD_LIMIT_BIT; \
} while (0)

#define GET_JOINT_NHL_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_MIN_HARD_LIMIT_BIT ? 1 : 0)

#define SET_JOINT_NHL_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_MIN_HARD_LIMIT_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_MIN_HARD_LIMIT_BIT; \
} while (0)

#define GET_JOINT_HOME_SWITCH_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_HOME_SWITCH_BIT ? 1 : 0)

#define SET_JOINT_HOME_SWITCH_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_HOME_SWITCH_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_HOME_SWITCH_BIT; \
} while (0)

#define GET_JOINT_HOMING_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_HOMING_BIT ? 1 : 0)

#define SET_JOINT_HOMING_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_HOMING_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_HOMING_BIT; \
} while (0)

#define GET_JOINT_HOMED_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_HOMED_BIT ? 1 : 0)

#define SET_JOINT_HOMED_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_HOMED_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_HOMED_BIT; \
} while (0)

#define GET_JOINT_AT_HOME_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_AT_HOME_BIT ? 1 : 0)

#define SET_JOINT_AT_HOME_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_AT_HOME_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_AT_HOME_BIT; \
} while (0)

#define GET_JOINT_FERROR_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_FERROR_BIT ? 1 : 0)

#define SET_JOINT_FERROR_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_FERROR_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_FERROR_BIT; \
} while (0)

#define GET_JOINT_FAULT_FLAG(joint) ((joint)->flag & EMCMOT_JOINT_FAULT_BIT ? 1 : 0)

#define SET_JOINT_FAULT_FLAG(joint,fl) do { \
    if (fl) (joint)->flag |= EMCMOT_JOINT_FAULT_BIT; \
    else (joint)->flag &= ~EMCMOT_JOINT_FAULT_BIT; \
} while (0)

#if defined(__KERNEL__)
#define HAVE_CPU_KHZ
#endif
#endif /* MOT_PRIV_H */
