
/*----------------------------------------------------------------------
This work derived from alex joni's halui.cc
Copyright: 2013,2014
Author:    Dewey Garrett <dgarrett@panix.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----------------------------------------------------------------------*/
#include "rcs_print.hh"
#include "emc.hh"
#include <stdio.h>
#include "hal.h"
#include "rtapi.h"
#include "inihal.hh"

static int debug=0;
static int comp_id;
extern value_inihal_data old_inihal_data;

static ptr_inihal_data *the_inihal_data;

#define PREFIX "ini."
#define EPSILON .00001
#define CLOSE(a,b,eps) ((a)-(b) < +(eps) && (a)-(b) > -(eps))


#define CATCH_AND_LOG_ERROR(call__, need_return__) \
do {\
    int res = call__; \
    if (0 != res) { \
        if (emc_debug & EMC_DEBUG_CONFIG) { \
            rcs_print("%s: %d: bad return value %d from %s\n", __FILE__, __LINE__, res, #call__); \
        } \
        if (need_return__) { \
            return -1; \
        } \
    } \
} while (0)

#define LOG_ERROR(call__) CATCH_AND_LOG_ERROR(call__, false)
#define RETURN_ON_ERROR(call__) CATCH_AND_LOG_ERROR(call__, true)

#define NEW(NAME) new_inihal_data.NAME
#define OLD(NAME) old_inihal_data.NAME

#define CHANGED(NAME) \
    ((old_inihal_data.NAME) - (new_inihal_data.NAME) > +(EPSILON)) \
    || \
    ((old_inihal_data.NAME) - (new_inihal_data.NAME) < -(EPSILON))

#define CHANGED_IDX(NAME,IDX) \
    ((old_inihal_data.NAME[IDX]) - (new_inihal_data.NAME[IDX]) > +(EPSILON)) \
    || \
    ((old_inihal_data.NAME[IDX]) - (new_inihal_data.NAME[IDX]) < -(EPSILON))

#define UPDATE(NAME) OLD(NAME) = NEW(NAME)

#define UPDATE_IDX(NAME,IDX) old_inihal_data.NAME[IDX] = new_inihal_data.NAME[IDX]

#define SHOW_CHANGE(NAME) do {\
    if (debug) { \
        fprintf(stderr,"Changed: "#NAME" %g-->%g\n",old_inihal_data.NAME, \
                                                    new_inihal_data.NAME); \
    } \
} while (0)

#define SHOW_CHANGE_ARC_BLEND() \
    fprintf(stderr,"Changed: blend_enable:          %d-->%d\n"\
                   "         blend_fallback_enable: %d-->%d\n"\
                   "         optimization_depth:    %d-->%d\n"\
                   "         gap_cycles:            %f-->%f\n"\
                   "         ramp_freq:             %f-->%f\n"\
           ,old_inihal_data.traj_arc_blend_enable \
           ,new_inihal_data.traj_arc_blend_enable \
           ,old_inihal_data.traj_arc_blend_fallback_enable \
           ,new_inihal_data.traj_arc_blend_fallback_enable \
           ,old_inihal_data.traj_arc_blend_optimization_depth \
           ,new_inihal_data.traj_arc_blend_optimization_depth \
           ,old_inihal_data.traj_arc_blend_gap_cycles \
           ,new_inihal_data.traj_arc_blend_gap_cycles \
           ,old_inihal_data.traj_arc_blend_ramp_freq \
           ,new_inihal_data.traj_arc_blend_ramp_freq \
          );

#define SHOW_CHANGE_IDX(NAME,IDX) do {\
    if (debug) { \
        fprintf(stderr,"Changed: "#NAME"[%d] %g-->%g\n",IDX,old_inihal_data.NAME[IDX], \
                                                            new_inihal_data.NAME[IDX]); \
    } \
} while (0)

#define MAKE_BIT_PIN(NAME,DIR) \
do { \
     retval = hal_pin_bit_newf(DIR,&(the_inihal_data->NAME),comp_id,PREFIX#NAME); \
     if (retval < 0) return retval; \
   } while (0)

#define MAKE_S32_PIN(NAME,DIR) \
do { \
     retval = hal_pin_s32_newf(DIR,&(the_inihal_data->NAME),comp_id,PREFIX#NAME); \
     if (retval < 0) return retval; \
   } while (0)

#define MAKE_U32_PIN(NAME,DIR) \
do { \
     retval = hal_pin_u32_newf(DIR,&(the_inihal_data->NAME),comp_id,PREFIX#NAME); \
     if (retval < 0) return retval; \
   } while (0)

#define MAKE_FLOAT_PIN(NAME,DIR) \
do { \
     retval = hal_pin_float_newf(DIR,&(the_inihal_data->NAME),comp_id,PREFIX#NAME); \
     if (retval < 0) return retval; \
   } while (0)

#define MAKE_FLOAT_PIN_IDX(NAME,DIR,IDX) \
do {                        \
     retval = hal_pin_float_newf(DIR,&(the_inihal_data->NAME[IDX]),\
                                 comp_id,PREFIX"%d."#NAME,IDX); \
     if (retval < 0) return retval; \
   } while (0)

#define INIT_PIN(NAME) *(the_inihal_data->NAME) = old_inihal_data.NAME;


int ini_hal_init(void)
{
    int retval;

    comp_id = hal_init("inihal");
    if (comp_id < 0) {
    rtapi_print_msg(RTAPI_MSG_ERR,
            "ini_hal_init: ERROR: hal_init() failed\n");
    return -1;
    }

    the_inihal_data = (ptr_inihal_data *) hal_malloc(sizeof(ptr_inihal_data));
    if (the_inihal_data == 0) {
        rtapi_print_msg(RTAPI_MSG_ERR,
                       "ini_hal_init: ERROR: hal_malloc() failed\n");
        hal_exit(comp_id);
        return -1;
    }

    for (int idx = 0; idx < EMCMOT_MAX_JOINTS; idx++) {
        MAKE_FLOAT_PIN_IDX(backlash,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(min_limit,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(max_limit,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(max_velocity,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(max_acceleration,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(ferror,HAL_IN,idx);
        MAKE_FLOAT_PIN_IDX(min_ferror,HAL_IN,idx);
    }

    MAKE_FLOAT_PIN(traj_default_velocity,HAL_IN);
    MAKE_FLOAT_PIN(traj_max_velocity,HAL_IN);
    MAKE_FLOAT_PIN(traj_max_angular_velocity,HAL_IN);
    MAKE_FLOAT_PIN(traj_default_acceleration,HAL_IN);
    MAKE_FLOAT_PIN(traj_max_acceleration,HAL_IN);

    MAKE_BIT_PIN(traj_arc_blend_enable,HAL_IN);
    MAKE_BIT_PIN(traj_arc_blend_fallback_enable,HAL_IN);
    MAKE_S32_PIN(traj_arc_blend_optimization_depth,HAL_IN);
    MAKE_FLOAT_PIN(traj_arc_blend_gap_cycles,HAL_IN);
    MAKE_FLOAT_PIN(traj_arc_blend_ramp_freq,HAL_IN);
    MAKE_FLOAT_PIN(traj_arc_blend_tangent_kink_ratio,HAL_IN);

    MAKE_U32_PIN(traj_biarc_max_iterations, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_velocity_rel_tolerance, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_velocity_abs_tolerance, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_deviation_abs_tolerance, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_feed_override_allowance, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_velocity_cutoff, HAL_IN);
    MAKE_FLOAT_PIN(traj_biarc_convergence_tolerance, HAL_IN);

    hal_ready(comp_id);
    return 0;
} // ini_hal_init()

int ini_hal_init_pins()
{
#define FIELD(t,f) INIT_PIN(f);
#define ARRAY(t,f,n) do { \
    for (int idx = 0; idx < n; idx++) { \
        INIT_PIN(f[idx]); \
    } \
} while (0);

HAL_FIELDS

#undef FIELD
#undef ARRAY
    return 0;
} // ini_hal_init_pins()

static void copy_hal_data(const ptr_inihal_data &i, value_inihal_data &j)
{
    int x;
#define FIELD(t,f) j.f = (i.f)?*i.f:0;
#define ARRAY(t,f,n) do { for (x = 0; x < n; x++) j.f[x] = (i.f[x])?*i.f[x]:0; } while (0);
    HAL_FIELDS
#undef FIELD
#undef ARRAY
} // copy_hal_data()

int check_ini_hal_items()
{
    value_inihal_data new_inihal_data_mutable;
    copy_hal_data(*the_inihal_data, new_inihal_data_mutable);
    const value_inihal_data &new_inihal_data = new_inihal_data_mutable;

    if (CHANGED(traj_default_velocity)) {
        SHOW_CHANGE(traj_default_velocity);
        UPDATE(traj_default_velocity);
        LOG_ERROR(emcTrajSetVelocity(0, NEW(traj_default_velocity)));
    }
    if (CHANGED(traj_max_velocity) | CHANGED(traj_max_angular_velocity)) {
        SHOW_CHANGE(traj_max_velocity);
        SHOW_CHANGE(traj_max_angular_velocity);
        UPDATE(traj_max_velocity);
        UPDATE(traj_max_angular_velocity);
        LOG_ERROR(emcTrajSetMaxVelocity(NEW(traj_max_velocity),
                                            NEW(traj_max_angular_velocity)));
    }
    if (CHANGED(traj_default_acceleration)) {
        SHOW_CHANGE(traj_default_acceleration);
        UPDATE(traj_default_acceleration);
        LOG_ERROR(emcTrajSetAcceleration(NEW(traj_default_acceleration)));
    }
    if (CHANGED(traj_max_acceleration)) {
        SHOW_CHANGE(traj_max_acceleration);
        UPDATE(traj_max_acceleration);
        LOG_ERROR(emcTrajSetMaxAcceleration(NEW(traj_max_acceleration)));
    }

    if (   CHANGED(traj_arc_blend_enable)
        || CHANGED(traj_arc_blend_fallback_enable)
        || CHANGED(traj_arc_blend_optimization_depth)
        || CHANGED(traj_arc_blend_gap_cycles)
        || CHANGED(traj_arc_blend_ramp_freq)
        || CHANGED(traj_arc_blend_tangent_kink_ratio)
       ) {
        SHOW_CHANGE_ARC_BLEND()
        UPDATE(traj_arc_blend_enable);
        UPDATE(traj_arc_blend_fallback_enable);
        UPDATE(traj_arc_blend_optimization_depth);
        UPDATE(traj_arc_blend_gap_cycles);
        UPDATE(traj_arc_blend_ramp_freq);
        UPDATE(traj_arc_blend_tangent_kink_ratio);
        tp_arc_blend_config_t cfg = {
            OLD(traj_arc_blend_enable),
            OLD(traj_arc_blend_fallback_enable),
            OLD(traj_arc_blend_optimization_depth),
            (int)OLD(traj_arc_blend_gap_cycles),
            OLD(traj_arc_blend_ramp_freq),
            OLD(traj_arc_blend_tangent_kink_ratio),
        };
        RETURN_ON_ERROR(emcSetupArcBlends(cfg));
    }

    if (   CHANGED(traj_biarc_max_iterations)
        || CHANGED(traj_biarc_velocity_cutoff)
        || CHANGED(traj_biarc_convergence_tolerance)
        || CHANGED(traj_biarc_velocity_abs_tolerance)
        || CHANGED(traj_biarc_velocity_rel_tolerance)
        || CHANGED(traj_biarc_deviation_abs_tolerance)
        || CHANGED(traj_biarc_feed_override_allowance)
       ) {

        UPDATE(traj_biarc_max_iterations);
        UPDATE(traj_biarc_velocity_cutoff);
        UPDATE(traj_biarc_convergence_tolerance);
        UPDATE(traj_biarc_velocity_abs_tolerance);
        UPDATE(traj_biarc_velocity_rel_tolerance);
        UPDATE(traj_biarc_deviation_abs_tolerance);
        UPDATE(traj_biarc_feed_override_allowance);
        biarc_solver_config_t cfg = {
            OLD(traj_biarc_max_iterations),
            OLD(traj_biarc_velocity_cutoff),
            OLD(traj_biarc_convergence_tolerance),
            OLD(traj_biarc_velocity_abs_tolerance),
            OLD(traj_biarc_velocity_rel_tolerance),
            OLD(traj_biarc_deviation_abs_tolerance),
            OLD(traj_biarc_feed_override_allowance),
        };
        RETURN_ON_ERROR(emcSetupBiArcSolverCfg(cfg));
    }

    for (int idx = 0; idx < EMCMOT_MAX_JOINTS; idx++) {
        if (CHANGED_IDX(backlash,idx) ) {
            SHOW_CHANGE_IDX(backlash,idx);
            UPDATE_IDX(backlash,idx);
            LOG_ERROR(emcAxisSetBacklash(idx,NEW(backlash[idx])));
        }
        if (CHANGED_IDX(min_limit,idx) ) {
            SHOW_CHANGE_IDX(min_limit,idx);
            UPDATE_IDX(min_limit,idx);
            LOG_ERROR(emcAxisSetMinPositionLimit(idx,NEW(min_limit[idx])));
        }
        if (CHANGED_IDX(max_limit,idx) ) {
            SHOW_CHANGE_IDX(max_limit,idx);
            UPDATE_IDX(max_limit,idx);
            LOG_ERROR(emcAxisSetMaxPositionLimit(idx,NEW(max_limit[idx])));
        }
        if (CHANGED_IDX(max_velocity,idx) ) {
            SHOW_CHANGE_IDX(max_velocity,idx);
            UPDATE_IDX(max_velocity,idx);
            LOG_ERROR(emcAxisSetMaxVelocity(idx, NEW(max_velocity[idx])));
        }
        if (CHANGED_IDX(max_acceleration,idx) ) {
            SHOW_CHANGE_IDX(max_acceleration,idx);
            UPDATE_IDX(max_acceleration,idx);
            LOG_ERROR(emcAxisSetMaxAcceleration(idx, NEW(max_acceleration[idx])));
        }
        if (CHANGED_IDX(ferror,idx) ) {
            SHOW_CHANGE_IDX(ferror,idx);
            UPDATE_IDX(ferror,idx);
            LOG_ERROR(emcAxisSetFerror(idx,NEW(ferror[idx])));
        }
        if (CHANGED_IDX(min_ferror,idx) ) {
            SHOW_CHANGE_IDX(min_ferror,idx);
            UPDATE_IDX(min_ferror,idx);
            LOG_ERROR(emcAxisSetMinFerror(idx,NEW(min_ferror[idx])));
        }
    }

    return 0;
} // check_ini_hal_items

// vim: sts=4 sw=4 et
