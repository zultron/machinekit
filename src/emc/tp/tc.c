/*!
********************************************************************
* Description: tc.c
*\brief Discriminate-based trajectory planning
*
*\author Derived from a work by Fred Proctor & Will Shackleford
*\author rewritten by Chris Radek
*
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/

#include "rtapi.h"		/* rtapi_print_msg */
#include "rtapi_math.h"
#include "posemath.h"
#include "blendmath.h"
#include "emcpose.h"
#include "tc.h"
#include "tp_types.h"
#include "spherical_arc.h"
#include "motion_types.h"

//Debug output
#include "tp_debug.h"
#include "tp_enums.h"

double tcGetMaxVelFromLength(TC_STRUCT const * const tc)
{
    double sample_maxvel = tc->target / (tc->cycle_time * TP_MIN_SEGMENT_CYCLES);
    return fmin(tc->maxvel_geom, sample_maxvel);
}

/**
 * Planning-time maximum velocity for the given segment.
 * This function ignores runtime conditions like if a segment is on final deceleration, actively blending, etc.
 */
double tcGetPlanMaxTargetVel(
    TC_STRUCT const * const tc,
    double max_feed_scale)
{
    double v_max = tcGetMaxVelFromLength(tc);
    double v_max_target = v_max;

    switch (tc->synchronized) {
        case TC_SYNC_NONE:
            // Get maximum reachable velocity from max feed override
            v_max_target = tc->reqvel * max_feed_scale;
            break;

        case TC_SYNC_VELOCITY: //Fallthrough
        case TC_SYNC_POSITION:
            // Assume no spindle override during blend target
            // max feed override here is variability of the spindle speed, rather than a user feed override
            v_max_target = tc->uu_per_rev * tc->tag.speed * max_feed_scale;
            break;
    }

    // Clip maximum velocity by the segment's own maximum velocity
    return fmin(v_max_target, v_max);
}

double tcGetAccelScale(const TC_STRUCT *tc)
{
    // Handle any acceleration reduction due to an approximate-tangent "blend" with the previous or next segment
    double a_scale = (1.0 - fmax(tc->kink_accel_reduce, tc->kink_accel_reduce_prev));
    return a_scale;
}

double tcGetOverallMaxAccel(const TC_STRUCT *tc)
{
    return tc->maxaccel * tcGetAccelScale(tc);
}

/**
 * Get acceleration for a tc based on the trajectory planner state.
 */
double tcGetTangentialMaxAccel(TC_STRUCT const * const tc)
{
    double a_scale = tcGetAccelScale(tc);

    // Reduce allowed tangential acceleration in circular motions to stay
    // within overall limits (accounts for centripetal acceleration while
    // moving along the circular path).
    if (tc->motion_type == TC_CIRCULAR || tc->motion_type == TC_SPHERICAL) {
        //Limit acceleration for cirular arcs to allow for normal acceleration
        a_scale *= tc->acc_ratio_tan;
    }
    return tc->maxaccel * a_scale;
}


int tcSetKinkProperties(TC_STRUCT *prev_tc, TC_STRUCT *tc, double kink_vel, double accel_reduction)
{
  // NOTE: use_kink field is not set until later, if we choose tangent blending without an arc connector
  prev_tc->kink_vel = kink_vel;
  prev_tc->kink_accel_reduce = fmax(accel_reduction, prev_tc->kink_accel_reduce);
  tc->kink_accel_reduce_prev = fmax(accel_reduction, tc->kink_accel_reduce_prev);

  return 0;
}

/**
 * Check if a segment can be consumed without disrupting motion or synced IO.
 */
int tcCanConsume(TC_STRUCT const * const tc)
{
    if (!tc) {
        return false;
    }

    if (tc->motion_type != TC_LINEAR) {
        return false;
    }

    if (tc->syncdio.anychanged || tc->atspeed) {
        //TODO add other conditions here (for any segment that should not be consumed by blending
        return false;
    }

    return true;
}

/**
 * Find the geometric tangent vector to a helical arc.
 * Unlike the acceleration vector, the result of this calculation is a vector
 * tangent to the helical arc. This is called by wrapper functions for the case of a circular or helical arc.
 */
int pmCircleTangentVector(PmCircle const * const circle,
        double angle_in, PmCartesian * const u_tan_out)
{

    // Cheat: "rHelix" already has helical height built in
    double cheat_dz = 1.0 / circle->angle;
    double dr = circle->spiral / circle->angle;
    PmCartesian tmp, uRadial, uTangential;

    const double Cangle_by_r = cos(angle_in) / circle->radius;
    const double Sangle_by_r = sin(angle_in) / circle->radius;

    pmCartScalMult(&circle->rTan, Cangle_by_r, &uRadial);
    pmCartScalMult(&circle->rPerp, Sangle_by_r, &tmp);
    pmCartCartAddEq(&uRadial, &tmp);

    pmCartScalMult(&circle->rTan, -Sangle_by_r, &uTangential);
    pmCartScalMult(&circle->rPerp, Cangle_by_r, &tmp);
    pmCartCartAddEq(&uTangential, &tmp);

    // Add spiral component (scaled by radius)
    pmCartScalMult(&uTangential, circle->radius + dr * angle_in, u_tan_out);
    pmCartScalMult(&uRadial, dr, &tmp);
    pmCartCartAddEq(u_tan_out, &tmp);

    // Add helical component (scaled by radius)
    pmCartScalMult(&circle->rHelix, cheat_dz, &tmp);
    pmCartCartAddEq(u_tan_out, &tmp);

    // Normalize to get a unit vector in tangent direction
    pmCartUnitEq(u_tan_out);
    return 0;
}


/**
 * Calulate the unit tangent vector at the start of a move for any segment.
 */
int tcGetStartTangentUnitVector(TC_STRUCT const * const tc, PmCartesian * const out) {

    switch (tc->motion_type) {
        case TC_LINEAR:
            *out=tc->coords.line.xyz.uVec;
            return 0;
        case TC_RIGIDTAP:
            *out = tc->coords.rigidtap.nominal_xyz.uVec;
            return 0;
        case TC_CIRCULAR:
            return pmCircleTangentVector(&tc->coords.circle.xyz, 0.0, out);
        case TC_SPHERICAL:
            return arcTangent(&tc->coords.arc.xyz, 0, out);
        case TC_DWELL:
            return -1;
    }
    rtapi_print_msg(RTAPI_MSG_ERR, "Invalid motion type %d!\n",tc->motion_type);
    return -1;
}

/**
 * Calulate the unit tangent vector at the end of a move for any segment.
 */
int tcGetEndTangentUnitVector(TC_STRUCT const * const tc, PmCartesian * const out) {

    switch (tc->motion_type) {
        case TC_LINEAR:
            *out=tc->coords.line.xyz.uVec;
            return 0;
        case TC_RIGIDTAP:
            // Final placement move goes back to the start point in the same direction
            *out = tc->coords.rigidtap.nominal_xyz.uVec;
            return 0;
        case TC_CIRCULAR:
            return pmCircleTangentVector(&tc->coords.circle.xyz,
                    tc->coords.circle.xyz.angle, out);
        case TC_SPHERICAL:
            return arcTangent(&tc->coords.arc.xyz, 1.0, out);
        case TC_DWELL:
            return -1;
    }
    rtapi_print_msg(RTAPI_MSG_ERR, "Invalid motion type %d!\n",tc->motion_type);
    return -1;
}

int tcGetTangentUnitVector(TC_STRUCT const * const tc, double progress, PmCartesian * const out) {

    switch (tc->motion_type) {
        case TC_LINEAR:
            *out = tc->coords.line.xyz.uVec;
            return TP_ERR_OK;
        case TC_RIGIDTAP:
            *out = tc->coords.rigidtap.nominal_xyz.uVec;
            return TP_ERR_OK;
        case TC_CIRCULAR:
        {
            double angle=0;
            pmCircleAngleFromProgress(
                &tc->coords.circle.xyz,
                &tc->coords.circle.fit,
                progress,
                &angle);
            pmCircleTangentVector(
                &tc->coords.circle.xyz,
                angle,
                out);
            return TP_ERR_OK;
        }
        default:
            rtapi_print_msg(RTAPI_MSG_ERR, "Invalid motion type %d!\n",tc->motion_type);
            return -1;
    }
}

/**
 * Calculate the distance left in the trajectory segment in the indicated
 * direction.
 */
double tcGetDistanceToGo(TC_STRUCT const * const tc, int direction)
{
    double distance;
    if (direction == TC_DIR_FORWARD) {
        // Return standard distance to go
        distance = tc->target - tc->progress;
    } else {
        // Reverse direction, distance from zero instead of target
        distance = tc->progress;
    }

    return distance;
}

double tcGetTarget(TC_STRUCT const * const tc, int direction)
{
    return (direction == TC_DIR_REVERSE) ? 0.0 : tc->target;
}

/*! tcGetPos() function
 *
 * \brief This function calculates the machine position along the motion's path.
 *
 * As we move along a TC, from zero to its length, we call this function repeatedly,
 * with an increasing tc->progress.
 * This function calculates the machine position along the motion's path
 * corresponding to the current progress.
 * It gets called at the end of tpRunCycle()
 *
 * @param    tc    the current TC that is being planned
 *
 * @return	 EmcPose   returns a position (\ref EmcPose = datatype carrying XYZABC information
 */

int tcGetPos(TC_STRUCT const * const tc, EmcPose * const out) {
    tcGetPosReal(tc, tc->progress, out);
    return 0;
}

int tcGetStartpoint(TC_STRUCT const * const tc, EmcPose * const pos)
{
    PmCartesian xyz;
    PmCartesian abc;
    PmCartesian uvw;

    switch (tc->motion_type){
        case TC_RIGIDTAP:
            xyz = tc->coords.rigidtap.nominal_xyz.start;
            abc = tc->coords.rigidtap.abc;
            uvw = tc->coords.rigidtap.uvw;
            break;
        case TC_LINEAR:
            xyz = tc->coords.line.xyz.start;
            abc = tc->coords.line.abc.start;
            uvw = tc->coords.line.uvw.start;
            break;
        case TC_CIRCULAR:
            pmCircleStartPoint(&tc->coords.circle.xyz, &xyz);
            abc = tc->coords.circle.abc.start;
            uvw = tc->coords.circle.uvw.start;
            break;
        case TC_SPHERICAL:
            arcPoint(&tc->coords.arc.xyz,
                    0,
                    &xyz);
            abc = tc->coords.arc.abc;
            uvw = tc->coords.arc.uvw;
            break;
        case TC_DWELL:
            *pos = tc->coords.dwell.dwell_pos;
            return TP_ERR_OK;
    }

    pmCartesianToEmcPose(&xyz, &abc, &uvw, pos);

    return TP_ERR_OK;
}

int tcGetEndpoint(TC_STRUCT const * const tc, EmcPose * const out) {
    tcGetPosReal(tc, tc->target, out);
    return 0;
}

int tcGetPosReal(TC_STRUCT const * const tc, double progress, EmcPose * const pos)
{
    PmCartesian xyz;
    PmCartesian abc;
    PmCartesian uvw;

    // Used for arc-length to angle conversion with spiral segments
    double angle = 0.0;
    int res_fit = TP_ERR_OK;

    switch (tc->motion_type){
        case TC_RIGIDTAP:
            pmCartLinePoint(&tc->coords.rigidtap.actual_xyz, progress, &xyz);
            // no rotary move allowed while tapping
            abc = tc->coords.rigidtap.abc;
            uvw = tc->coords.rigidtap.uvw;
            break;
        case TC_LINEAR:
            pmCartLinePoint(&tc->coords.line.xyz,
                    progress * tc->coords.line.xyz.tmag / tc->target,
                    &xyz);
            pmCartLinePoint(&tc->coords.line.uvw,
                    progress * tc->coords.line.uvw.tmag / tc->target,
                    &uvw);
            pmCartLinePoint(&tc->coords.line.abc,
                    progress * tc->coords.line.abc.tmag / tc->target,
                    &abc);
            break;
        case TC_CIRCULAR:
            res_fit = pmCircleAngleFromProgress(&tc->coords.circle.xyz,
                    &tc->coords.circle.fit,
                    progress, &angle);
            pmCirclePoint(&tc->coords.circle.xyz,
                    angle,
                    &xyz);
            pmCartLinePoint(&tc->coords.circle.abc,
                    progress * tc->coords.circle.abc.tmag / tc->target,
                    &abc);
            pmCartLinePoint(&tc->coords.circle.uvw,
                    progress * tc->coords.circle.uvw.tmag / tc->target,
                    &uvw);
            break;
        case TC_SPHERICAL:
            arcPoint(&tc->coords.arc.xyz,
                    progress,
                    &xyz);
            abc = tc->coords.arc.abc;
            uvw = tc->coords.arc.uvw;
            break;
        case TC_DWELL:
            *pos = tc->coords.dwell.dwell_pos;
            return TP_ERR_OK;
    }

    if (res_fit == TP_ERR_OK) {
        // Don't touch pos unless we know the value is good
        pmCartesianToEmcPose(&xyz, &abc, &uvw, pos);
    }
    return res_fit;
}


/**
 * Set the terminal condition (i.e. blend or stop) for the given motion segment.
 * Also sets flags on the next segment relevant to blending (e.g. parabolic blend sets the blend_prev flag).
 */
int tcSetTermCond(TC_STRUCT *tc, tc_term_cond_t term_cond)
{
    if (!tc) {
        return -1;
    }

    tc->term_cond = term_cond;
    return 0;
}

int tcFindBlendTolerance(TC_STRUCT const * const prev_tc,
        TC_STRUCT const * const tc, double * const T_blend)
{
    const double tolerance_ratio = 0.25;
    double T1 = prev_tc->tolerance;
    double T2 = tc->tolerance;
    //Detect zero tolerance = no tolerance and force to reasonable maximum
    if (T1 <= 0) {
        T1 = prev_tc->nominal_length * tolerance_ratio;
    }
    if (T2 <= 0) {
        T2 = tc->nominal_length * tolerance_ratio;
    }
    double nominal_tolerance = fmin(T1,T2);
    //Blend tolerance is the limit of what we can reach by blending alone,
    //consuming half a segment or less (parabolic equivalent)
    double blend_tolerance = fmin(fmin(nominal_tolerance,
                prev_tc->nominal_length * tolerance_ratio),
            tc->nominal_length * tolerance_ratio);
    *T_blend = blend_tolerance;
    return 0;
}


double pmLine9Target(PmLine9 * const line9, int pure_angular)
{
    if (pure_angular && !line9->abc.tmag_zero) {
        return line9->abc.tmag;
    } else if (!line9->xyz.tmag_zero) {
        return line9->xyz.tmag;
    } else if (!line9->uvw.tmag_zero) {
        return line9->uvw.tmag;
    } else if (!line9->abc.tmag_zero) {
        return line9->abc.tmag;
    } else {
        rtapi_print_msg(RTAPI_MSG_DBG,"line can't have zero length! xyz start = %.12e,%.12e,%.12e, end = %.12e,%.12e,%.12e\n",
                line9->xyz.start.x,
                line9->xyz.start.y,
                line9->xyz.start.z,
                line9->xyz.end.x,
                line9->xyz.end.y,
                line9->xyz.end.z);
        return 0.0;
    }
}


/**
 * Initialize a new trajectory segment with common parameters.
 *
 * NOTE: this function only sets default values that are non-zero. Make sure
 * the struct is properly initialized BEFORE calling this function.
 */
int tcInit(TC_STRUCT * const tc,
        tc_motion_type_t motion_type,
        int canon_motion_type,
        double cycle_time,
        unsigned char enables,
        char atspeed)
{

    /** Motion type setup */
    tc->motion_type = motion_type;
    tc->canon_motion_type = canon_motion_type;
    tc->atspeed = atspeed;

    /** Segment settings passed down from interpreter*/
    tc->enables = enables;
    tc->cycle_time = cycle_time;

    /** Segment settings (given values later during setup / optimization) */
    tc->indexrotary = -1;

    tc->active_depth = 1;

    tc->acc_ratio_tan = BLEND_ACC_RATIO_TANGENTIAL;

    return TP_ERR_OK;
}

/**
 * Set kinematic properties for a trajectory segment.
 */
int tcSetupMotion(TC_STRUCT * const tc,
        double vel,
        double ini_maxvel,
        double acc)
{
    //FIXME assumes that state is already set up in TC_STRUCT, which depends on external order of function calls.

    tc->maxaccel = acc;

    tc->maxvel_geom = ini_maxvel;

    tc->reqvel = vel;
    // Initial guess at target velocity is just the requested velocity
    tc->target_vel = vel;

    return TP_ERR_OK;
}

int tcSetupState(TC_STRUCT * const tc, TP_STRUCT const * const tp)
{
    tc->tolerance = tp->tolerance;
    tc->synchronized = tp->synchronized;
    tc->uu_per_rev = tp->uu_per_rev;
    return TP_ERR_OK;
}

int pmLine9Init(PmLine9 * const line9,
        EmcPose const * const start,
        EmcPose const * const end)
{
    // Scratch variables
    PmCartesian start_xyz, end_xyz;
    PmCartesian start_uvw, end_uvw;
    PmCartesian start_abc, end_abc;

    // Convert endpoint to cartesian representation
    emcPoseToPmCartesian(start, &start_xyz, &start_abc, &start_uvw);
    emcPoseToPmCartesian(end, &end_xyz, &end_abc, &end_uvw);

    // Initialize cartesian line members
    int xyz_fail = pmCartLineInit(&line9->xyz, &start_xyz, &end_xyz);
    int abc_fail = pmCartLineInit(&line9->abc, &start_abc, &end_abc);
    int uvw_fail = pmCartLineInit(&line9->uvw, &start_uvw, &end_uvw);

#ifdef TP_DEBUG
    print_json5_log_start(pmLine9Init);
    print_json5_PmCartLine_("xyz", line9->xyz);
    print_json5_PmCartLine_("abc", line9->abc);
    print_json5_PmCartLine_("uvw", line9->abc);
    print_json5_end_();
#endif

    if (xyz_fail || abc_fail || uvw_fail) {
        rtapi_print_msg(RTAPI_MSG_ERR,"Failed to initialize Line9, err codes %d, %d, %d\n",
                xyz_fail,abc_fail,uvw_fail);
        return TP_ERR_FAIL;
    }
    return TP_ERR_OK;
}

int pmCircle9Init(PmCircle9 * const circ9,
        EmcPose const * const start,
        EmcPose const * const end,
        PmCartesian const * const center,
        PmCartesian const * const normal,
        int turn)
{
    PmCartesian start_xyz, end_xyz;
    PmCartesian start_uvw, end_uvw;
    PmCartesian start_abc, end_abc;

    emcPoseToPmCartesian(start, &start_xyz, &start_abc, &start_uvw);
    emcPoseToPmCartesian(end, &end_xyz, &end_abc, &end_uvw);

    int xyz_fail = pmCircleInit(&circ9->xyz, &start_xyz, &end_xyz, center, normal, turn);
    //Initialize line parts of Circle9
    int abc_fail = pmCartLineInit(&circ9->abc, &start_abc, &end_abc);
    int uvw_fail = pmCartLineInit(&circ9->uvw, &start_uvw, &end_uvw);

    int res_fit = findSpiralArcLengthFit(&circ9->xyz,&circ9->fit);

    if (xyz_fail || abc_fail || uvw_fail || res_fit) {
        rtapi_print_msg(RTAPI_MSG_ERR,"Failed to initialize Circle9, err codes %d, %d, %d, %d\n",
                xyz_fail, abc_fail, uvw_fail, res_fit);
        return TP_ERR_FAIL;
    }
    return TP_ERR_OK;
}

double pmCircle9Target(PmCircle9 const * const circ9)
{

    double h2;
    pmCartMagSq(&circ9->xyz.rHelix, &h2);
    double helical_length = pmSqrt(pmSq(circ9->fit.total_planar_length) + h2);

    return helical_length;
}

int tcUpdateCircleAccRatio(TC_STRUCT * tc, double v_max)
{
    if (tc->motion_type == TC_CIRCULAR) {
        PmCircleLimits limits = pmCircleActualMaxVel(&tc->coords.circle.xyz,
                             v_max,
                             tc->acc_normal_max * tcGetAccelScale(tc));
        tc->maxvel_geom = limits.v_max;
        tc->acc_ratio_tan = limits.acc_ratio;
        return 0;
    }
    // TODO handle blend arc here too?
    return 1; //nothing to do, but not an error
}

/**
 * "Finalizes" a segment so that its length can't change.
 * By setting the finalized flag, we tell the optimizer that this segment's
 * length won't change anymore. Since any blends are already set up, we can
 * trust that the length will be the same, and so can use the length in the
 * velocity optimization.
 */
int tcFinalizeLength(TC_STRUCT * const tc, double max_feed_override)
{
    //Apply velocity corrections
    if (!tc) {
        return TP_ERR_FAIL;
    }

    if (tc->finalized) {
        tp_debug_print("tc %d already finalized\n", tc->id);
        return TP_ERR_NO_ACTION;
    }
#ifdef TP_DEBUG
    double maxvel_old = tc->maxvel_geom;
#endif

    tcUpdateCircleAccRatio(tc, tcGetPlanMaxTargetVel(tc, max_feed_override));

    if (tc->term_cond == TC_TERM_COND_PARABOLIC) {
        tcSetTermCond(tc, TC_TERM_COND_EXACT);
    }
    tc->finalized = 1;

#ifdef TP_DEBUG
    print_json5_log_start(tpFinalizeLength);
    print_json5_tc_id_data_(tc);
    print_json5_string_("motion_type", tcMotionTypeAsString((tc_motion_type_t)tc->motion_type));
    print_json5_double(maxvel_old);
    print_json5_double_("maxvel", tc->maxvel_geom);
    print_json5_double_("acc_ratio_tan", tc->acc_ratio_tan);
    print_json5_log_end();
#endif

    tc->finalized = 1;
    return TP_ERR_OK;
}

int pmRigidTapInit(PmRigidTap * const tap,
        EmcPose const * const start,
        EmcPose const * const end,
        double reversal_scale)
{
    PmCartesian start_xyz, end_xyz;
    PmCartesian abc, uvw;

    //Slightly more allocation this way, but much easier to read
    emcPoseToPmCartesian(start, &start_xyz, &abc, &uvw);
    emcPoseGetXYZ(end, &end_xyz);

    // Setup XYZ motion
    pmCartLineInit(&tap->nominal_xyz, &start_xyz, &end_xyz);
    tap->actual_xyz = tap->nominal_xyz;

    // Copy over fixed ABC and UVW points
    tap->abc = abc;
    tap->uvw = uvw;

    // Setup initial tap state
    tap->reversal_target = tap->nominal_xyz.tmag;
    tap->reversal_scale = reversal_scale;
    tap->state = RIGIDTAP_TAPPING;

    return TP_ERR_OK;
}

/** Returns true if segment has ONLY rotary motion, false otherwise. */
int tcPureRotaryCheck(TC_STRUCT const * const tc)
{
    return (tc->motion_type == TC_LINEAR) &&
        (tc->coords.line.xyz.tmag_zero) &&
        (tc->coords.line.uvw.tmag_zero);
}

/**
 * Given a PmCircle and a circular segment, copy the circle in as the XYZ portion of the segment, then update the motion parameters.
 * NOTE: does not yet support ABC or UVW motion!
 */
int tcSetCircleXYZ(TC_STRUCT * const tc, PmCircle const * const circ)
{

    //Update targets with new arc length
    if (!circ || tc->motion_type != TC_CIRCULAR) {
        return TP_ERR_FAIL;
    }
    if (!tc->coords.circle.abc.tmag_zero || !tc->coords.circle.uvw.tmag_zero) {
        rtapi_print_msg(RTAPI_MSG_ERR, "SetCircleXYZ does not supportABC or UVW motion\n");
        return TP_ERR_FAIL;
    }

    // Store the new circular segment (or use the current one)

    if (!circ) {
        rtapi_print_msg(RTAPI_MSG_ERR, "SetCircleXYZ missing new circle definition\n");
        return TP_ERR_FAIL;
    }

    tc->coords.circle.xyz = *circ;
    // Update the arc length fit to this new segment
    findSpiralArcLengthFit(&tc->coords.circle.xyz, &tc->coords.circle.fit);

    // compute the new total arc length using the fit and store as new
    // target distance
    tc->target = pmCircle9Target(&tc->coords.circle);

    return TP_ERR_OK;
}

// Helper functions to convert enums to pretty-print for debug output

const char *tcTermCondAsString(tc_term_cond_t c)
{
    switch (c)
    {
        case TC_TERM_COND_STOP:
            return "EXACT_STOP";
        case TC_TERM_COND_EXACT:
            return "EXACT_PATH";
        case TC_TERM_COND_PARABOLIC:
            return "PARABOLIC";
        case TC_TERM_COND_TANGENT:
            return "TANGENT";
    }
    return "NONE";
}

const char *tcMotionTypeAsString(tc_motion_type_t c)
{
    switch (c)
    {
        case TC_LINEAR:
            return "Linear";
        case TC_CIRCULAR:
            return "Circular";
        case TC_RIGIDTAP:
            return "RigidTap";
        case TC_SPHERICAL:
            return "SphericalArc";
        case TC_DWELL:
            return "Dwell";
    }
    return "NONE";
}

const char *tcSyncModeAsString(tc_spindle_sync_t c)
{
    switch (c)
    {
        case TC_SYNC_NONE:
            return "sync_none";
        case TC_SYNC_VELOCITY:
            return "sync_velocity";
        case TC_SYNC_POSITION:
            return "sync_position";
    }
    return "NONE";
}
