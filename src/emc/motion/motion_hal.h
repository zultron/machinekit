#ifndef MOTION_HAL_H
#define MOTION_HAL_H

/***********************************************************************
*                       TYPEDEFS, ENUMS, ETC.                          *
************************************************************************/

/* First we define structures for data shared with the HAL */

/* HAL visible data notations:
   RPA:  read only parameter
   WPA:  write only parameter
   WRPA: read/write parameter
   RPI:  read only pin
   WPI:  write only pin
   WRPI: read/write pin
*/

/* joint data */
#include "hal.h"
#include "emcmotcfg.h"

typedef struct {
    hal_float_t *coarse_pos_cmd;/* RPI: commanded position, w/o comp */
    hal_float_t *joint_vel_cmd;	/* RPI: commanded velocity, w/o comp */
    hal_float_t *backlash_corr;	/* RPI: correction for backlash */
    hal_float_t *backlash_filt;	/* RPI: filtered backlash correction */
    hal_float_t *backlash_vel;	/* RPI: backlash speed variable */
    hal_float_t *downstream_offset; /* WRPI: offset being applied to the axis by some component past motion (in HAL) */
                                    /* HAL component sets: An offset is being applied to axis on top of pos_cmd */
                                    /* motmod clears: Offset value has been incorparated into pos_cmd, HAL comp should stop applying */
    hal_bit_t *continuous_output;   /* RPI: Normally 1, zero on this pin indicates a step in motor_pos_output */
    hal_float_t *motor_offset;	/* RPI: motor offset, for checking homing stability */
    hal_float_t *motor_pos_cmd;	/* WPI: commanded position, with comp */
    hal_float_t *motor_pos_fb;	/* RPI: position feedback, with comp */
    hal_float_t *joint_pos_cmd;	/* WPI: commanded position w/o comp, mot ofs */
    hal_float_t *joint_pos_fb;	/* RPI: position feedback, w/o comp */
    hal_float_t *f_error;	/* RPI: following error */
    hal_float_t *f_error_lim;	/* RPI: following error limit */

    hal_float_t *free_pos_cmd;	/* RPI: free traj planner pos cmd */
    hal_float_t *free_vel_lim;	/* RPI: free traj planner vel limit */
    hal_bit_t *free_tp_enable;	/* RPI: free traj planner is running */
    hal_bit_t *kb_jog_active;   /* RPI: executing keyboard jog */
    hal_bit_t *wheel_jog_active;/* RPI: executing handwheel jog */

    hal_bit_t *active;		/* RPI: joint is active, whatever that means */
    hal_bit_t *in_position;	/* RPI: joint is in position */
    hal_bit_t *error;		/* RPI: joint has an error */
    hal_bit_t *phl;		/* RPI: joint is at positive hard limit */
    hal_bit_t *nhl;		/* RPI: joint is at negative hard limit */
    hal_bit_t *homing;		/* RPI: joint is homing */
    hal_bit_t *homed;		/* RPI: joint was homed */
    hal_bit_t *f_errored;	/* RPI: joint had too much following error */
    hal_bit_t *faulted;		/* RPI: joint amp faulted */
    hal_bit_t *pos_lim_sw;	/* RPI: positive limit switch input */
    hal_bit_t *neg_lim_sw;	/* RPI: negative limit switch input */
    hal_bit_t *home_sw;		/* RPI: home switch input */
    hal_bit_t *index_enable;	/* RPIO: motmod sets: request reset on index
                         encoder clears: index arrived */
    hal_bit_t *amp_fault;	/* RPI: amp fault input */
    hal_bit_t *amp_enable;	/* WPI: amp enable output */
    hal_s32_t *home_state;	/* WPI: homing state machine state */

    hal_bit_t *unlock;          /* WPI: command that axis should unlock for rotation */
    hal_bit_t *is_unlocked;     /* RPI: axis is currently unlocked */

    hal_s32_t *jog_counts;	/* WPI: jogwheel position input */
    hal_bit_t *jog_enable;	/* RPI: enable jogwheel */
    hal_float_t *jog_scale;	/* RPI: distance to jog on each count */
    hal_bit_t *jog_vel_mode;	/* RPI: true for "velocity mode" jogwheel */

} joint_hal_t;

/* machine data */

typedef struct {
    hal_bit_t *probe_input;	/* RPI: probe switch input */
    hal_bit_t *setter_input;
    hal_bit_t *discrete_setter_input;
    hal_bit_t *enable;		/* RPI: motion inhibit input */
    hal_bit_t *spindle_index_enable;
    hal_bit_t *spindle_is_atspeed;
    hal_float_t *spindle_revs;
    hal_bit_t *spindle_inhibit;	/* RPI: set TRUE to stop spindle (non maskable)*/
    hal_float_t *pos_tracking_error; //!< Reports the position "error" term from position-synched motion (not the true spindle tracking error, just the control input)
    hal_s32_t *pos_tracking_mode; // Position tracking "mode", controls which tracking algorithm to use
    hal_s32_t *pos_tracking_state; // Internal state of position tracking controller
    hal_float_t *pos_tracking_velocity; // target feed velocity according to position tracker used during tpRunCycle
    hal_s32_t *rigid_tap_state; // Internal state of rigid tapping state machine
    hal_float_t *rigid_tap_overshoot; // Reports how much the rigid tapping cycle overshot it's reversal target (negative means it didn't reach the target)
    hal_float_t *rigid_tap_reversal_vel_rps; // Reports how much the rigid tapping cycle overshot it's reversal target (negative means it didn't reach the target)
    hal_float_t *adaptive_feed;	/* RPI: adaptive feedrate, 0.0 to 1.0 */
    hal_bit_t *feed_hold;	/* RPI: set TRUE to stop motion maskable with g53 P1*/
    hal_bit_t *feed_inhibit;	/* RPI: set TRUE to stop motion (non maskable)*/
    hal_bit_t *motion_enabled;	/* RPI: motion enable for all joints */
    hal_bit_t *in_position;	/* RPI: all joints are in position */
//    hal_bit_t *inpos_output;	/* WPI: all joints are in position (used to power down steppers for example) */
    hal_bit_t *coord_mode;	/* RPA: TRUE if coord, FALSE if free */
    hal_bit_t *teleop_mode;	/* RPA: TRUE if teleop mode */
    hal_bit_t *coord_error;	/* RPA: TRUE if coord mode error */
    hal_bit_t *on_soft_limit;	/* RPA: TRUE if outside a limit */

    hal_s32_t *program_line;    /* RPA: program line causing current motion */
    hal_s32_t *motion_type;	/* RPA: type (feed/rapid) of currently commanded motion */
    hal_s32_t *next_program_line;    /* program line next to execute */
    hal_s32_t *completed_program_line;    /* program line most recently completed */
    hal_s32_t *queued_program_line;    /* program line most recently queued*/
    hal_s32_t *mot_command;       /* command being processed in the current update*/
    hal_s32_t *mot_command_count; /* count of total commands executed by motion (will overflow)*/
    hal_float_t *current_vel;   /* RPI: velocity magnitude in machine units */
    hal_float_t *requested_vel;   /* RPI: requested velocity magnitude in machine units / second */
    hal_float_t *gcode_feed;      /* RPI: gcode F-word velocity magnitude in machine units */
    hal_float_t *distance_to_go;/* RPI: distance to go in current move*/
    hal_bit_t *waiting_on_spindle; /* RPI: TRUE when spindle-synched
                    * move is waiting on spindle */

    hal_bit_t debug_bit_0;	/* RPA: generic param, for debugging */
    hal_bit_t debug_bit_1;	/* RPA: generic param, for debugging */
    hal_float_t debug_float_0;	/* RPA: generic param, for debugging */
    hal_float_t debug_float_1;	/* RPA: generic param, for debugging */
    hal_float_t debug_float_2;	/* RPA: generic param, for debugging */
    hal_float_t debug_float_3;	/* RPA: generic param, for debugging */
    hal_float_t *debug_gain_0;	/* WPI: generic param, for debugging */
    hal_float_t *debug_gain_1;	/* WPI: generic param, for debugging */
    hal_s32_t debug_s32_0;	/* RPA: generic param, for debugging */
    hal_s32_t debug_s32_1;	/* RPA: generic param, for debugging */

    hal_bit_t *synch_do[EMCMOT_MAX_DIO]; /* WPI array: output pins for motion synched IO */
    hal_bit_t *synch_di[EMCMOT_MAX_DIO]; /* RPI array: input pins for motion synched IO */
    hal_float_t *analog_input[EMCMOT_MAX_AIO]; /* RPI array: input pins for analog Inputs */
    hal_float_t *analog_output[EMCMOT_MAX_AIO]; /* RPI array: output pins for analog Inputs */


    // creating a lot of pins for spindle control to be very flexible
    // the user needs only a subset of these

    // simplest way of spindle control (output start/stop)
    hal_bit_t *spindle_on;	/* spindle spin output */

    // same thing for 2 directions
    hal_bit_t *spindle_forward;	/* spindle spin-forward output */
    hal_bit_t *spindle_reverse;	/* spindle spin-reverse output */

    // simple velocity control (as long as the output is active the spindle
    //                          should accelerate/decelerate
    hal_bit_t *spindle_incr_speed;	/* spindle spin-increase output */
    hal_bit_t *spindle_decr_speed;	/* spindle spin-decrease output */

    // simple output for brake
    hal_bit_t *spindle_brake;	/* spindle brake output */

    hal_float_t *spindle_max_acceleration_rps2; // Allows spindle to tell motion how fast it can decelerate
    hal_float_t *spindle_cmd_latency_sec; // Allows spindle to tell motion how fast it can decelerate

    // output of a prescribed speed (to hook-up to a velocity controller)
    hal_float_t *spindle_speed_out;	/* spindle speed output */
    hal_float_t *spindle_speed_out_rps;	/* spindle speed output */
    hal_float_t *spindle_speed_out_abs;	/* spindle speed output absolute*/
    hal_float_t *spindle_speed_out_rps_abs;	/* spindle speed output absolute*/
    hal_float_t *spindle_speed_cmd_rps;	/* spindle speed command without SO applied */
    hal_float_t *spindle_speed_in;	/* spindle speed measured in rps from external source such as a high-frequency encoder reader*/
    hal_float_t *spindle_speed_in_estimate;	/* spindle speed estimated internally (used if spindle_speed_in is not connected) */

    // spindle orient
    hal_float_t *spindle_orient_angle;	/* out: desired spindle angle, degrees */
    hal_s32_t   *spindle_orient_mode;	/* out: 0: least travel; 1: cw; 2: ccw */
    hal_bit_t   *spindle_orient;	/* out: signal orient in progress */
    hal_bit_t   *spindle_locked;	/* out: signal orient complete, spindle locked */
    hal_bit_t   *spindle_is_oriented;	/* in: orientation completed */
    hal_s32_t   *spindle_orient_fault;	/* in: error code of failed operation */

    // FIXME - debug only, remove later
    hal_float_t traj_pos_out;	/* RPA: traj internals, for debugging */
    hal_float_t traj_vel_out;	/* RPA: traj internals, for debugging */
    hal_u32_t traj_active_tc;	/* RPA: traj internals, for debugging */
    hal_float_t tc_pos[4];	/* RPA: traj internals, for debugging */
    hal_float_t tc_vel[4];	/* RPA: traj internals, for debugging */
    hal_float_t tc_acc[4];	/* RPA: traj internals, for debugging */

    // realtime overrun detection
    hal_u32_t last_period;	/* param: last period in clocks */
    hal_float_t last_period_ns;	/* param: last period in nanoseconds */

    hal_float_t *tooloffset_x;
    hal_float_t *tooloffset_y;
    hal_float_t *tooloffset_z;
    hal_float_t *tooloffset_a;
    hal_float_t *tooloffset_b;
    hal_float_t *tooloffset_c;
    hal_float_t *tooloffset_u;
    hal_float_t *tooloffset_v;
    hal_float_t *tooloffset_w;

    joint_hal_t joint[EMCMOT_MAX_JOINTS];	/* data for each joint */

} emcmot_hal_data_t;
#endif // MOTION_HAL_H
