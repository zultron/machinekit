/********************************************************************
* Description: blendmath.c
*   Circular arc blend math functions
*
* Author: Robert W. Ellenberg
* License: GPL Version 2
* System: Linux
*   
* Copyright (c) 2014 All rights reserved.
*
* Last change:
********************************************************************/

#include "posemath.h"
#include "tc_types.h"
#include "tc.h"
#include "tp_types.h"
#include "rtapi_math.h"
#include "spherical_arc.h"
#include "blendmath.h"
#include "tp_debug.h"
#include "tp_enums.h"
#include "stdlib.h"
#include "math_util.h"

// Inspired by interp
#define CHP(callee__) \
do { \
    tp_err_t res = (callee__); \
    if (res != TP_ERR_OK) { \
        return res; \
    } \
} while (0)

static const double NEAR_INFINITY=1e300;

/** @section utilityfuncs Utility functions */

/**
 * Find the maximum angle allowed between "tangent" segments.
 * @param v speed of motion in worst case (i.e. at max feed).
 * @param acc magnitude of acceleration allowed during "kink".
 *
 * Since we are discretized by a timestep, the maximum allowable
 * "kink" in a trajectory is bounded by normal acceleration. A small
 * kink will effectively be one step along the tightest radius arc
 * possible at a given speed.
 */
double findMaxTangentAngle(double v_plan, double acc_limit, double cycle_time)
{
    //Find acc hiccup we're allowed to get
    //TODO somewhat redundant with findKinkAccel, should refactor
    double acc_margin = BLEND_ACC_RATIO_NORMAL * BLEND_KINK_FACTOR * acc_limit;
    double dx = v_plan / cycle_time;
    return fabs(acc_margin / fmax(dx, TP_POS_EPSILON));
}


/**
 * Find the acceleration required to create a specific change in path
 * direction, assuming constant speed.
 * This determines how much of a "spike" in acceleration will occur due to a
 * slight mismatch between tangent directions at the start / end of a segment.
 */
double findKinkAccel(double kink_angle, double v_plan, double cycle_time)
{
    double dx = v_plan / cycle_time;
    return fabs(dx * kink_angle);
}


/**
 * Sign function that returns a valid numerical result for sign(0), rather than NaN.
 */
double fsign(double f)
{
    if (f>0) {
        return 1.0;
    } else if (f < 0) {
        return -1.0;
    } else {
        //Technically this should be NAN but that's a useless result for tp purposes
        return 0;
    }
}

/** negate a value (or not) based on a bool parameter */
static inline double negate(double f, int neg)
{
    return (neg) ? -f : f;
}

/** Clip the input at the specified minimum (in place). */
int clip_min(double * const x, double min) {
    if ( *x < min ) {
        *x = min;
        return 1;
    }
    return 0;
}


/** Clip the input at the specified maximum (in place). */
int clip_max(double * const x, double max) {
    if ( *x > max ) {
        *x = max;
        return 1;
    }
    return 0;
}

/**
 * Saturate a value x to be within +/- max.
 */
double saturate(double x, double max) {
    if ( x > max ) {
        return max;
    }
    else if ( x < (-max) ) {
        return -max;
    }
    else {
        return x;
    }
}


/**
 * Apply bounds to a value x.
 */
double bound(double x, double max, double min) {
    if ( x > max ) {
        return max;
    }
    else if ( x < (min) ) {
        return min;
    }
    else {
        return x;
    }
}


/** In-place saturation function */
int sat_inplace(double * const x, double max) {
    if ( *x > max ) {
        *x = max;
        return 1;
    }
    else if ( *x < -max ) {
        *x = -max;
        return -1;
    }
    return 0;
}

#if 0
static int pmCirclePrint(PmCircle const * const circ) {
    tp_debug_print(" center = %f %f %f\n",
            circ->center.x,
            circ->center.y,
            circ->center.z);
    tp_debug_print(" radius = %f\n", circ->radius);
    tp_debug_print(" spiral = %f\n", circ->spiral);
    tp_debug_print(" angle = %f\n", circ->angle);
    //TODO add other debug data here as needed
    return TP_ERR_OK;
}
#endif

/**
 * @section geomfuncs Geometry check functions
 */

/**
 * Calculate the best-fit circle to the spiral segment.
 * @param circ spiral to be approximated
 * @param base_pt the point about which the circle is fit
 * @param u_tan tangent unit vector at the base point of the approximation
 * @param[out] center_out displaced center for circular approximation
 * @param[out] radius_out adjusted radius
 *
 * The adjusted center for the circle fit is found by displacing the center
 * along the spiral tangent by the spiral coefficient. The adjusted radius is
 * the distance between the base point and this new center.
 *
 */
static inline int findSpiralApproximation(PmCircle const * const circ,
        PmCartesian const * const base_pt,
        PmCartesian const * const u_tan,
        PmCartesian * const center_out,
        double * const radius_out)
{
    double dr = circ->spiral / circ->angle;

    /*tp_debug_print("In findSpiralApproximation\n");*/
    /*tp_debug_print(" dr = %f\n",dr);*/
    /*tp_debug_print(" utan = %f %f %f\n",*/
            /*u_tan->x,*/
            /*u_tan->y,*/
            /*u_tan->z);*/
    pmCartScalMult(u_tan, dr, center_out);
    /*tp_debug_print(" circcenter = %f %f %f\n",*/
            /*circ->center.x,*/
            /*circ->center.y,*/
            /*circ->center.z);*/
    pmCartCartAddEq(center_out, &circ->center);

    PmCartesian r_adjust;
    pmCartCartSub(base_pt, center_out, &r_adjust);
    pmCartMag(&r_adjust, radius_out);

    tp_debug_print(" adjusted center = %f %f %f\n",
            center_out->x,
            center_out->y,
            center_out->z);

    tp_debug_print(" adjusted radius = %f\n", *radius_out);

    return TP_ERR_OK;
}


/**
 * Calculate the angle to trim from a circle based on the blend geometry.
 *
 * @param P intersection point
 * @param arc_center calculated center of blend arc
 * @param center actual center of circle (not spiral approximated center)
 * @return trim angle
 */
static inline double findTrimAngle(PmCartesian const * const P,
        PmCartesian const * const arc_center,
        PmCartesian const * const center)
{
    //Define vectors relative to circle center
    PmCartesian u_P;
    pmCartCartSub(P, center, &u_P);
    pmCartUnitEq(&u_P);

    PmCartesian u_arccenter;
    pmCartCartSub(arc_center, center, &u_arccenter);
    pmCartUnitEq(&u_arccenter);

    double dot;
    pmCartCartDot(&u_arccenter, &u_P, &dot);
    double dphi = acos(saturate(dot,1.0));
    tp_debug_print(" dphi = %g\n",dphi);
    return dphi;
}


/**
 * Verify that a blend arc is tangent to a circular arc.
 */
int checkTangentAngle(PmCircle const * const circ, SphericalArc const * const arc, BlendGeom3 const * const geom, BlendParameters const * const param, double cycle_time, int at_end)
{
    // Debug Information to diagnose tangent issues
    PmCartesian u_circ, u_arc;
    double t = at_end ? 1.0 : 0.0;
    arcTangent(arc, t, &u_arc);

    if (at_end) {
        pmCircleTangentVector(circ, 0, &u_circ);
    } else {
        pmCircleTangentVector(circ, circ->angle, &u_circ);
    }

    pmCartUnitEq(&u_arc);

    // Find angle between tangent unit vectors
    double dot;
    pmCartCartDot(&u_circ, &u_arc, &dot);
    double blend_angle = acos(saturate(dot,1.0));

    // Check against the maximum allowed tangent angle for the given velocity and acceleration
    double angle_max = findMaxTangentAngle(param->v_plan, param->a_max_planar, cycle_time);

    tp_debug_print("tangent angle = %f, max = %f\n",
            blend_angle,
            angle_max);

    tp_debug_print("circ_tan = [%g %g %g]\n",
            u_circ.x,
            u_circ.y,
            u_circ.z);
    tp_debug_print("arc_tan = [%g %g %g]\n",
            u_arc.x,
            u_arc.y,
            u_arc.z);

    PmCartesian diff;
    pmCartCartSub(&u_arc,&u_circ,&diff);
    tp_debug_print("diff = [%g %g %g]\n",
            diff.x,
            diff.y,
            diff.z);

    if (blend_angle > angle_max) {
        tp_debug_print("angle too large\n");
        return TP_ERR_FAIL;
    }

    return TP_ERR_OK;
}


/**
 * Checks if two UNIT vectors are parallel to the given angle tolerance (in radians).
 * @warning tol depends on the small angle approximation and will not be
 * accurate for angles larger than about 10 deg. This function is meant for
 * small tolerances!
 */
int pmCartCartParallel(PmCartesian const * const u1,
                       PmCartesian const * const u2,
                       double tol)
{
    double d_diff;
    {
        PmCartesian u_diff;
        pmCartCartSub(u1, u2, &u_diff);
        pmCartMagSq(&u_diff, &d_diff);
    }

#if defined(TP_DEBUG) && defined(TP_PEDANTIC)
    print_json5_log_start(pmCartCartParallel, Check);
    print_json5_double(d_diff);
    print_json5_end_();
#endif

    return d_diff < tol;
}

/**
 * Checks if two UNIT vectors are anti-parallel to the given angle tolerance (in radians).
 * @warning tol depends on the small angle approximation and will not be
 * accurate for angles larger than about 10 deg. This function is meant for
 * small tolerances!
 */
int pmCartCartAntiParallel(PmCartesian const * const u1,
                           PmCartesian const * const u2,
                           double tol)
{
    double d_sum;
    {
        PmCartesian u_sum;
        pmCartCartAdd(u1, u2, &u_sum);
        pmCartMagSq(&u_sum, &d_sum);
    }

#if defined(TP_DEBUG) && defined(TP_PEDANTIC)
    print_json5_log_start(pmCartCartAntiParallel, Check);
    print_json5_double(d_sum);
    print_json5_end_();
#endif

    return d_sum < tol;
}


/**
 * Check if two cartesian vectors are parallel or anti-parallel
 * The input tolerance specifies what the maximum angle between the
 * lines containing two vectors is. Note that vectors pointing in
 * opposite directions are still considered parallel, since their
 * containing lines are parallel.
 * @param u1 input unit vector 1
 * @param u2 input unit vector 2
 * @pre BOTH u1 and u2 must be unit vectors or calculation may be skewed.
 */
int pmUnitCartsColinear(PmCartesian const * const u1,
        PmCartesian const * const u2)
{
    return pmCartCartParallel(u1, u2, TP_ANGLE_EPSILON_SQ) || pmCartCartAntiParallel(u1, u2, TP_ANGLE_EPSILON_SQ);
}


/**
 * Somewhat redundant function to calculate the segment intersection angle.
 * The intersection angle is half of the supplement of the "divergence" angle
 * between unit vectors. If two unit vectors are pointing in the same
 * direction, then the intersection angle is PI/2. This is based on the
 * simple_tp formulation for tolerances.
 */
double findIntersectionAngle(PmCartesian const * const u1,
        PmCartesian const * const u2)
{
    double dot;
    pmCartCartDot(u1, u2, &dot);

    sat_inplace(&dot,1.0);

    return acos(-dot)/2.0;
}

double findIntersectionTanAngle(
    PmCartesian const * const u1,
    PmCartesian const * const u2)
{
    double dot;
    pmCartCartDot(u1, u2, &dot);

    double cos_2theta = -dot;

    return pmSqrt((1. - cos_2theta) / (1. + cos_2theta));
}

/** Calculate the minimum of the three values in a PmCartesian. */
double pmCartMin(PmCartesian const * const in)
{
    return fmin(fmin(in->x,in->y),in->z);
}

/**
 * Calculate the diameter of a circle incscribed on a central cross section of a 3D
 * rectangular prism.
 *
 * @param normal normal direction of plane slicing prism (must be unit length!).
 * @param extents distance from center to one corner of the prism.
 * @param diameter diameter of inscribed circle on cross section.
 *
 */
int calculateInscribedRadius(PmCartesian const * const normal,
        PmCartesian const * const bounds, double * const radius)
{
    if (!normal ) {
        return TP_ERR_MISSING_INPUT;
    }

    double n_mag;
    pmCartMagSq(normal, &n_mag);
    double mag_err = fabs(1.0 - n_mag);
    if (mag_err > pmSqrt(TP_POS_EPSILON)) {
        /*rtapi_print_msg(RTAPI_MSG_ERR,"normal vector <%.12g,%.12f,%.12f> has magnitude error = %e\n",*/
                /*normal->x,*/
                /*normal->y,*/
                /*normal->z,*/
                /*mag_err);*/
        return TP_ERR_FAIL;
    }

    PmCartesian planar_x,planar_y,planar_z;

    //Find perpendicular component of unit directions
    // FIXME Assumes normal is unit length
    
    /* This section projects the X / Y / Z unit vectors onto the plane
     * containing the motions. The operation is done "backwards" here due to a
     * quirk with posemath. 
     *
     */
    pmCartScalMult(normal, -normal->x, &planar_x);
    pmCartScalMult(normal, -normal->y, &planar_y);
    pmCartScalMult(normal, -normal->z, &planar_z);

    planar_x.x += 1.0;
    planar_y.y += 1.0;
    planar_z.z += 1.0;

    pmCartAbs(&planar_x, &planar_x);
    pmCartAbs(&planar_y, &planar_y);
    pmCartAbs(&planar_z, &planar_z);

    // Crude way to prevent divide-by-zero-error
    planar_x.x = fmax(planar_x.x,TP_POS_EPSILON);
    planar_y.y = fmax(planar_y.y,TP_POS_EPSILON);
    planar_z.z = fmax(planar_z.z,TP_POS_EPSILON);

    double x_scale, y_scale, z_scale;
    pmCartMag(&planar_x, &x_scale);
    pmCartMag(&planar_y, &y_scale);
    pmCartMag(&planar_z, &z_scale);

    double x_extent=0, y_extent=0, z_extent=0;
    if (bounds->x > 0) {
        x_extent = bounds->x / x_scale;
    }
    if (bounds->y > 0) {
        y_extent = bounds->y / y_scale;
    }
    if (bounds->z > 0) {
        z_extent = bounds->z / z_scale;
    }

    // Find the highest value to start from
    *radius = fmax(fmax(x_extent, y_extent),z_extent);

    // Only for active axes, find the minimum extent
    if (bounds->x > 0) {
        *radius = fmin(*radius, x_extent);
    }
    if (bounds->y > 0) {
        *radius = fmin(*radius, y_extent);
    }
    if (bounds->z > 0) {
        *radius = fmin(*radius, z_extent);
    }

    return TP_ERR_OK;
}

int findAccelScale(PmCartesian const * const acc,
        PmCartesian const * const bounds,
        PmCartesian * const scale)
{
    if (!acc || !bounds ) {
        return TP_ERR_MISSING_INPUT;
    }

    if (!scale ) {
        return TP_ERR_MISSING_OUTPUT;
    }

    // Find the scale of acceleration vs. machine accel bounds
    if (bounds->x != 0) {
    scale->x = fabs(acc->x / bounds->x);
    } else {
        scale->x = 0;
    }
    if (bounds->y != 0) {
    scale->y = fabs(acc->y / bounds->y);
    } else {
        scale->y = 0;
    }

    if (bounds->z != 0) {
    scale->z = fabs(acc->z / bounds->z);
    } else {
        scale->z = 0;
    }

    return TP_ERR_OK;
}

// KLUDGE this is a messy workaround for named fields in PmCartesian
static inline void minBoundRatioIfNonzero(double s, double b, double *m2)
{
    if (s > 0.0 && b > 0.0) {
        //Have to square b here since the scale is also squared
        *m2 = fmin(*m2, pmSq(b) / s);
    }
}

/**
 * Finds the maximum allowed value of the bounded quantity on the given plane.
 *
 * @param plane_envelope_sq is the square of the maximal value of the ith component of a unit
 * vector rotated through the plane. This basically specifies the largest
 * possible contribution of each axis to a direction in the plane. We use squared values here
 * because they are easier to compute, and require only one square root at the end.
 * @param bounds Specifies the per-axis maximum value (e.g. acceleration, velocity)
 * @param max_planar_value maximum length of a vector in the plane that is always within limits
 * @return -1 on error, 0 if completed successfully
 */
int findMaxValueOnPlane(PmCartesian const * plane_envelope_sq,
        PmCartesian const * bounds,
        double * max_planar_value)
{
    if (!plane_envelope_sq || !bounds || !max_planar_value) {
        return -1;
    }
    double m2 = NEAR_INFINITY;

    minBoundRatioIfNonzero(plane_envelope_sq->x, bounds->x, &m2);
    minBoundRatioIfNonzero(plane_envelope_sq->y, bounds->y, &m2);
    minBoundRatioIfNonzero(plane_envelope_sq->z, bounds->z, &m2);

    // One square root at the end to get the actual max accel
    *max_planar_value = pmSqrt(m2);
    return 0;
}

/** Find real roots of a quadratic equation in standard form. */
int quadraticFormula(double A, double B, double C, double * const root0,
        double * const root1)
{
    if (A < 0.0) {
        A *=-1;
        B *=-1;
        C *=-1;
    }
    double disc = pmSq(B) - 4.0 * A * C;
    if (disc < 0) {
        tp_debug_print("discriminant %.12g < 0, A=%.12g, B=%.12g,C=%.12g\n", disc, A, B, C);
        return TP_ERR_FAIL;
    }
    double t1 = pmSqrt(disc);
    if (root0) {
        *root0 = ( -B + t1) / (2.0 * A);
    }
    if (root1) {
        *root1 = ( -B - t1) / (2.0 * A);
    }
    return TP_ERR_OK;
}

/**
 * @section blending blend math functions
 */

/**
 * Setup common geom parameters based on trajectory segments.
 * This function populates the geom structure and "input" fields of
 * the blend parameter structure. It returns an error if the segments
 * are not coplanar, or if one or both segments is not a circular arc.
 *
 * @param geom Stores simplified geometry used to calculate blend params.
 * @param prev_tc first linear move to blend
 * @param tc second linear move to blend
 */
int blendGeom3Init(BlendGeom3 * const geom,
        TC_STRUCT const * const prev_tc,
        TC_STRUCT const * const tc)
{
    geom->v_max1 = prev_tc->maxvel;
    geom->v_max2 = tc->maxvel;

    // Get tangent unit vectors to each arc at the intersection point
    int res_u1 = tcGetEndTangentUnitVector(prev_tc, &geom->u_tan1);
    int res_u2 = tcGetStartTangentUnitVector(tc, &geom->u_tan2);

    // Initialize u1 and u2 by assuming they match the tangent direction
    geom->u1 = geom->u_tan1;
    geom->u2 = geom->u_tan2;

    int res_intersect = tcGetIntersectionPoint(prev_tc, tc, &geom->P);

    tp_debug_print("Intersection point P = %f %f %f\n",
            geom->P.x,
            geom->P.y,
            geom->P.z);

    // Find angle between tangent vectors
    geom->theta_tan = findIntersectionAngle(&geom->u_tan1,
            &geom->u_tan2);

    // Test for intersection angle errors
    if(PM_PI / 2.0 - geom->theta_tan < TP_ANGLE_EPSILON) {
        tp_debug_print("Intersection angle too close to pi/2, can't compute normal\n");
        return TP_ERR_TOLERANCE;
    }

    if(geom->theta_tan < TP_ANGLE_EPSILON) {
        tp_debug_print("Intersection angle too small for arc fit\n");
        return TP_ERR_TOLERANCE;
    }

    blendCalculateNormals3(geom);

    return res_u1 |
        res_u2 |
        res_intersect;
}

void findVMaxByAltitude(
    PmCartesian const * const u1,
    PmCartesian const * const u2,
    BlendParameters * const param)
{
    if (fabs(param->phi) < TP_ANGLE_EPSILON) {
        param->v_max_altitude = fmin(param->v_max1, param->v_max2);
        return;
    }
    // Clip the angle at a reasonable value (less than 90 deg), to prevent div by zero
    double phi_effective = fmin(param->phi, PM_PI * 0.49);

    // Copy over maximum velocities, clipping velocity to place altitude within base
    double Cp = cos(phi_effective);
    double v_effective1 = fmin(param->v_max1, param->v_max2 / Cp);
    double v_effective2 = fmin(param->v_max2, param->v_max1 / Cp);

    tp_debug_print("v_max1 = %f, v_max2 = %f\n", v_effective1, v_effective2);

    // Get "altitude"
    double v_area = v_effective1 * v_effective2 / 2.0 * sin(param->phi);
    tp_debug_print("phi = %f\n", param->phi);
    tp_debug_print("v_area = %f\n", v_area);

    // Get "base" of triangle
    PmCartesian tmp1, tmp2, diff;
    pmCartScalMult(u1, v_effective1, &tmp1);
    pmCartScalMult(u2, v_effective2, &tmp2);
    pmCartCartSub(&tmp2, &tmp1, &diff);
    double base;
    pmCartMag(&diff, &base);
    tp_debug_print("v_base = %f\n", base);

    param->v_max_altitude = 2.0 * v_area / base;
}

static inline double normalAccelFromMax(double a_max)
{
    return a_max * BLEND_ACC_RATIO_NORMAL;
}

int blendParamKinematics(
    PmCartesian const * const u_tan1,
    PmCartesian const * const u_tan2,
    BlendParameters * const param,
    PmCartesian const * const acc_bound,
    PmCartesian const * const vel_bound)
{
    // KLUDGE: common operations, but not exactly kinematics
    param->phi = (PM_PI - param->theta * 2.0);

    int res_limits = find_blend_vel_accel_planar_limits(
        u_tan1,
        u_tan2,
        acc_bound,
        vel_bound,
        &param->a_max_planar,
        &param->v_max_planar);

    // Store max normal acceleration
    param->a_n_max = normalAccelFromMax(param->a_max_planar);

    // WARNING: assumes that we start from a valid v_max (either 0 or from the 'altitude' check)
    double v_max = fmax(param->v_max_planar, param->v_max_altitude);

    param->v_goal = fmin(param->v_goal, v_max);

    return res_limits;
}

int blendParamInitVelocities(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double maxFeedScale,
    BlendParameters * const param)
{
    // Copy over maximum velocities
    param->v_max1 = prev_tc->maxvel;
    param->v_max2 = tc->maxvel;

    // Find the nominal velocity for the blend segment with no overrides
    double v_req_prev = tcGetMaxTargetVel(prev_tc, 1.0);
    double v_req_this = tcGetMaxTargetVel(tc, 1.0);
    tp_debug_print("vr_prev = %f, vr_this = %f\n", v_req_prev, v_req_this);
    param->v_req = fmax(v_req_prev, v_req_this);

    // Find the worst-case velocity we should reach for either segment
    param->v_goal = fmax(tcGetMaxTargetVel(prev_tc, maxFeedScale),
            tcGetMaxTargetVel(tc, maxFeedScale));

    return 0;
}

/**
 * Setup blend paramaters based on two linear segments.
 * This function populates the geom structure and "input" fields of the blend parameter structure based.
 * @param geom Stores simplified geometry used to calculate blend params.
 * @param param Abstracted parameters for blending calculations
 * @param prev_tc first linear move to blend
 * @param tc second linear move to blend
 * @param acc_bound maximum X, Y, Z machine acceleration
 * @param vel_bound maximum X, Y, Z machine velocity
 * @param maxFeedScale maximum allowed feed override (set in INI)
 */
int blendInit3FromLineLine(BlendGeom3 * const geom, BlendParameters * const param,
        TC_STRUCT const * const prev_tc,
        TC_STRUCT const * const tc,
        PmCartesian const * const acc_bound,
        PmCartesian const * const vel_bound,
        double maxFeedScale)
{

    if (tc->motion_type != TC_LINEAR || prev_tc->motion_type != TC_LINEAR) {
        return TP_ERR_FAIL;
    }

    int res_init = blendGeom3Init(geom, prev_tc, tc);
    if (res_init != TP_ERR_OK) {
        return res_init;
    }

    param->theta = geom->theta_tan;

    tp_debug_print("theta = %f\n", param->theta);

    param->phi = (PM_PI - param->theta * 2.0);

    blendGeom3Print(geom);

    //Nominal length restriction prevents gobbling too much of parabolic blends
    param->L1 = fmin(prev_tc->target, prev_tc->nominal_length * BLEND_DIST_FRACTION);
    param->L2 = tc->target * BLEND_DIST_FRACTION;
    tp_debug_print("prev. nominal length = %f, next nominal_length = %f\n",
            prev_tc->nominal_length, tc->nominal_length);
    tp_debug_print("L1 = %f, L2 = %f\n", param->L1, param->L2);

    // Setup common parameters
    int res_ini_vel = blendParamInitVelocities(prev_tc, tc, maxFeedScale, param);
    tcFindBlendTolerance(prev_tc, tc, &param->tolerance);

    findVMaxByAltitude(
        &geom->u1,
        &geom->u2,
        param);

    int res_kin = blendParamKinematics(
        &geom->u_tan1,
        &geom->u_tan2,
        param,
        acc_bound,
        vel_bound);

    return res_kin | res_ini_vel;
}


/**
 * Calculate plane normal and binormal based on unit direction vectors.
 */
int blendCalculateNormals3(BlendGeom3 * const geom)
{

    int err_cross = pmCartCartCross(&geom->u_tan1,
            &geom->u_tan2,
            &geom->binormal);
    int err_unit_b = pmCartUnitEq(&geom->binormal);

    tp_debug_print("binormal = [%f %f %f]\n", geom->binormal.x,
            geom->binormal.y,
            geom->binormal.z);

    pmCartCartSub(&geom->u_tan2, &geom->u_tan1, &geom->normal);
    int err_unit_n = pmCartUnitEq(&geom->normal);

    tp_debug_print("normal = [%f %f %f]\n", geom->normal.x,
            geom->normal.y,
            geom->normal.z);
    return (err_cross || err_unit_b || err_unit_n);
}

/**
 * Compute blend parameters based on line data.
 * Blend arc parameters such as radius and velocity are calculated here. These
 * parameters are later used to create the actual arc geometry in other
 * functions.
 */
int blendComputeParameters(BlendParameters * const param)
{

    // Find maximum distance h from arc center to intersection point
    double h_tol = param->tolerance / (1.0 - sin(param->theta));

    // Find maximum distance along lines allowed by tolerance
    double d_tol = cos(param->theta) * h_tol;
    tp_debug_print(" d_tol = %f\n", d_tol);

    // Find minimum distance by blend length constraints
    double d_lengths = fmin(param->L1, param->L2);
    double d_geom = fmin(d_lengths, d_tol);
    // Find radius from the limiting length
    double R_geom = tan(param->theta) * d_geom;

    // Find maximum velocity allowed by accel and radius
    double v_normal = pmSqrt(param->a_n_max * R_geom);
    tp_debug_print("v_normal = %f\n", v_normal);

    param->v_plan = fmin(v_normal, param->v_goal);

    /*Get the limiting velocity of the equivalent parabolic blend. We use the
     * time it would take to do a "stock" parabolic blend as a metric for how
     * much of the segment to consume. A long segment will have a high
     * "triangle" velocity, so the radius will only be as large as is needed to
     * reach the cornering speed. A short segment will have a low triangle
     * velocity, much lower than the actual curvature limit, which can be used
     * to calculate an equivalent blend radius.
     * */
    double a_parabolic = param->a_max_planar * 0.5;
    double v_triangle = pmSqrt(2.0 * a_parabolic  * d_geom);
    double t_blend = fmin(v_triangle, param->v_plan) / (a_parabolic);
    double s_blend = t_blend * param->v_plan;
    double R_blend = fmin(s_blend / param->phi, R_geom);   //Clamp by limiting radius

    param->R_plan = fmax(pmSq(param->v_plan) / param->a_n_max, R_blend);
    param->d_plan = param->R_plan / tan(param->theta);

    tp_debug_print("v_plan = %f\n", param->v_plan);
    tp_debug_print("R_plan = %f\n", param->R_plan);
    tp_debug_print("d_plan = %f\n", param->d_plan);

    /* "Actual" velocity means the velocity when feed override is 1.0.  Recall
     * that v_plan may be greater than v_req by the max feed override. If our
     * worst-case planned velocity is higher than the requested velocity, then
     * clip at the requested velocity. This allows us to increase speed above
     * the feed override limits.
     */
    if (param->v_plan > param->v_req) {
        param->v_actual = param->v_req;
    } else {
        param->v_actual = param->v_plan;
    }

    // Store arc length of blend arc for future checks
    param->s_arc = param->R_plan * param->phi;

    if (param->R_plan < TP_POS_EPSILON) {
        tp_debug_print("#Blend radius too small, aborting arc\n");
        return TP_ERR_FAIL;
    }

    if (param->s_arc < TP_MIN_ARC_LENGTH) {
        tp_debug_print("#Blend arc length too small, aborting arc\n");
        return TP_ERR_FAIL;
    }
    return TP_ERR_OK;
}


/** Check if the previous line segment will be consumed based on the blend arc parameters. */
int blendCheckConsume(BlendParameters * const param,
        BlendPoints3 const * const points,
        TC_STRUCT const * const prev_tc, int gap_cycles)
{
    //Initialize values
    param->consume = 0;
    param->line_length = 0;
    if (!prev_tc) {
        return -1;
    }

    if (prev_tc->motion_type != TC_LINEAR) {
        return 0;
    }

    //Check for segment length limits
    double L_prev = prev_tc->target - points->trim1;
    double prev_seg_time = L_prev / param->v_plan;

    bool can_consume = tcCanConsume(prev_tc);
    param->consume = (prev_seg_time < gap_cycles * prev_tc->cycle_time && can_consume);
    if (param->consume) {
        tp_debug_print("consuming prev line, L_prev = %g\n",
                L_prev);
        param->line_length = L_prev;
    }
    return 0;
}


/**
 * Compute spherical arc points based on blend arc data.
 * Once blend parameters are computed, the three arc points are calculated
 * here.
 */
int blendFindPoints3(BlendPoints3 * const points, BlendGeom3 const * const geom,
        BlendParameters const * const param)
{
    // Find center of blend arc along normal vector
    double center_dist = param->R_plan / sin(param->theta);
    tp_debug_print("center_dist = %f\n", center_dist);

    pmCartScalMult(&geom->normal, center_dist, &points->arc_center);
    pmCartCartAddEq(&points->arc_center, &geom->P);
    tp_debug_print("arc_center = %f %f %f\n",
            points->arc_center.x,
            points->arc_center.y,
            points->arc_center.z);

    // Start point is d_plan away from intersection P in the
    // negative direction of u1
    pmCartScalMult(&geom->u1, -param->d_plan, &points->arc_start);
    pmCartCartAddEq(&points->arc_start, &geom->P);
    tp_debug_print("arc_start = %f %f %f\n",
            points->arc_start.x,
            points->arc_start.y,
            points->arc_start.z);

    // End point is d_plan away from intersection P in the
    // positive direction of u1
    pmCartScalMult(&geom->u2, param->d_plan, &points->arc_end);
    pmCartCartAddEq(&points->arc_end, &geom->P);
    tp_debug_print("arc_end = %f %f %f\n",
            points->arc_end.x,
            points->arc_end.y,
            points->arc_end.z);

    //For line case, just copy over d_plan since it's the same
    points->trim1 = param->d_plan;
    points->trim2 = param->d_plan;

    return TP_ERR_OK;
}

/**
 * Setup the spherical arc struct based on the blend arc data.
 */
int arcFromBlendPoints3(SphericalArc * const arc, BlendPoints3 const * const points,
        BlendGeom3 const * const geom, BlendParameters const * const param)
{
    // If we consume the previous line, the remaining line length gets added here
    arc->uTan = geom->u_tan1;
    arc->line_length = param->line_length;

    // Create the arc from the processed points
    return arcInitFromPoints(arc, &points->arc_start,
            &points->arc_end, &points->arc_center);
}

int blendGeom3Print(BlendGeom3 const * const geom)
{
    tp_debug_print("u1 = %f %f %f\n",
            geom->u1.x,
            geom->u1.y,
            geom->u1.z);

    tp_debug_print("u2 = %f %f %f\n",
            geom->u2.x,
            geom->u2.y,
            geom->u2.z);
    return 0;
}

int blendPoints3Print(BlendPoints3 const * const points)
{
    tp_debug_print("arc_start = %f %f %f\n",
            points->arc_start.x,
            points->arc_start.y,
            points->arc_start.z);

    tp_debug_print("arc_center = %f %f %f\n",
            points->arc_center.x,
            points->arc_center.y,
            points->arc_center.z);

    tp_debug_print("arc_end = %f %f %f\n",
            points->arc_end.x,
            points->arc_end.y,
            points->arc_end.z);

    return 0;

}

double pmCartAbsMax(PmCartesian const * const v)
{
    return fmax(fmax(fabs(v->x),fabs(v->y)),fabs(v->z));
}


PmCircleLimits pmCircleActualMaxVel(PmCircle const * circle,
        double v_max,
        double a_max)
{
    double a_n_max_cutoff = BLEND_ACC_RATIO_NORMAL * a_max;

    // For debugging only
    double eff_radius = pmCircleEffectiveMinRadius(circle);

    // Find the acceleration necessary to reach the maximum velocity
    double a_n_vmax = pmSq(v_max) / fmax(eff_radius, DOUBLE_FUZZ);
    // Find the maximum velocity that still obeys our desired tangential / total acceleration ratio
    double v_max_cutoff = pmSqrt(a_n_max_cutoff * eff_radius);

    double v_max_actual = v_max;
    double acc_ratio_tan = BLEND_ACC_RATIO_TANGENTIAL;

    if (a_n_vmax > a_n_max_cutoff) {
        v_max_actual = v_max_cutoff;
    } else {
        acc_ratio_tan = pmSqrt(1.0 - pmSq(a_n_vmax / a_max));
    }

#ifdef TP_DEBUG
    print_json5_log_start(pmCircleActualMaxVel, Check);
    print_json5_double_("angle", circle->angle);
    print_json5_double_("nominal_radius", circle->radius);
    double r_spiral = spiralEffectiveRadius(circle);
    print_json5_double_("spiral_radius", r_spiral);
    print_json5_double(eff_radius);
    print_json5_double(v_max);
    print_json5_double(v_max_cutoff);
    print_json5_double(a_max);
    print_json5_double(a_n_max_cutoff);
    print_json5_double(a_n_vmax);
    print_json5_double(acc_ratio_tan);
    print_json5_log_end();
#endif

    PmCircleLimits limits = {
        v_max_actual,
        acc_ratio_tan
    };

    return limits;
}


/** @section spiralfuncs Functions to approximate spiral arc length */

/**
 * Intermediate function to find the angle for a parameter from 0..1 along the
 * spiral arc.
 */
static int pmCircleAngleFromParam(PmCircle const * const circle,
        SpiralArcLengthFit const * const fit,
        double t,
        double * const angle)
{
    if (fit->spiral_in) {
        t = 1.0 - t;
    }
    //TODO error or cleanup input to prevent param outside 0..1
    double s_in = t * fit->total_planar_length;

    // Quadratic formula to invert arc length -> angle

    double A = fit->b0;
    double B = fit->b1;
    double C = -s_in;

    double disc = pmSq(B) - 4.0 * A * C ;
    if (disc < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "discriminant %f is negative in angle calculation\n",disc);
        return TP_ERR_FAIL;
    }

    /*
     * Stability of inverting the arc-length relationship.
     * Since the b1 coefficient is analogous to arc radius, we can be
     * reasonably assured that it will be large enough not to cause numerical
     * errors. If this is not the case, then the arc itself is degenerate (very
     * small radius), and this condition should be caught well before here.
     *
     * Since an arc with a very small spiral coefficient will have a small b0
     * coefficient in the fit, we use the Citardauq Formula to ensure that the
     * positive root does not lose precision due to subtracting near-similar values.
     *
     * For more information, see:
     * http://people.csail.mit.edu/bkph/articles/Quadratics.pdf
     */

    double angle_out = (2.0 * C) / ( -B - pmSqrt(disc));

    if (fit->spiral_in) {
        // Spiral fit assumes that we're spiraling out, so
        // parameterize from opposite end
        angle_out = circle->angle - angle_out;
    }

    *angle = angle_out;
    return TP_ERR_OK;
}

static void printSpiralArcLengthFit(SpiralArcLengthFit const * const fit,
                                    double min_radius,
                                    double total_angle,
                                    double spiral_coef)
{
#ifdef TP_DEBUG
    print_json5_log_start(SpiralFit, Command);
    print_json5_double(min_radius);
    print_json5_double(total_angle);
    print_json5_double(spiral_coef);
    print_json5_double_("b0", fit->b0);
    print_json5_double_("b1", fit->b1);
    print_json5_double_("total_planar_length", fit->total_planar_length);
    print_json5_bool_("spiral_in", fit->spiral_in);
    print_json5_log_end();
#endif
}

/**
 * Approximate the arc length function of a general spiral.
 *
 * The closed-form arc length of a general archimedean spiral is rather
 * computationally messy to work with. 
 * See http://mathworld.wolfram.com/ArchimedesSpiral.html for the actual form.
 *
 * The simplification here is made possible by a few assumptions:
 *  1) That the spiral starts with a nonzero radius
 *  2) The spiral coefficient (i.e. change in radius / angle) is not too large
 *  3) The spiral coefficient has some minimum magnitude ("perfect" circles are handled as a special case)
 *
 * The 2nd-order fit below works by matching slope at the start and end of the
 * arc length vs. angle curve. This completely specifies the 2nd order fit.
 * Also, this fit predicts a total arc length >= the true arc length, which
 * means the true speed along the curve will be the same or slower than the
 * nominal speed.
 */
int findSpiralArcLengthFit(PmCircle const * const circle,
        SpiralArcLengthFit * const fit)
{
    // Additional data for arc length approximation
    double spiral_coef = circle->spiral / circle->angle;
    double min_radius = circle->radius;

    if (fsign(circle->spiral) < 0.0) {
        // Treat as positive spiral, parameterized in opposite
        // direction
        spiral_coef*=-1.0;
        // Treat final radius as starting radius for fit, so we add the
        // negative spiral term to get the minimum radius
        //
        min_radius+=circle->spiral;
        fit->spiral_in = true;
    } else {
        fit->spiral_in = false;
    }

    //Compute the slope of the arc length vs. angle curve at the start and end of the segment
    double slope_start = pmSqrt(pmSq(min_radius) + pmSq(spiral_coef));
    double slope_end = pmSqrt(pmSq(min_radius + spiral_coef * circle->angle) + pmSq(spiral_coef));

    fit->b0 = (slope_end - slope_start) / (2.0 * circle->angle);
    fit->b1 = slope_start;

    fit->total_planar_length = fit->b0 * pmSq(circle->angle) + fit->b1 * circle->angle;
    printSpiralArcLengthFit(fit, min_radius, circle->angle, spiral_coef);

    // Check against start and end angle
    double angle_end_chk = 0.0;
    int res_angle = pmCircleAngleFromParam(circle, fit, 1.0, &angle_end_chk);
    if (res_angle != TP_ERR_OK) {
        //TODO better error message
        rtapi_print_msg(RTAPI_MSG_ERR,
                "Spiral fit failed\n");
        return TP_ERR_FAIL;
    }

    // Check fit against angle
    double fit_err = angle_end_chk - circle->angle;
    if (fabs(fit_err) > TP_ANGLE_EPSILON) {
        rtapi_print_msg(RTAPI_MSG_ERR,
                "Spiral fit angle difference is %e, maximum allowed is %e\n",
                fit_err,
                TP_ANGLE_EPSILON);
        return TP_ERR_FAIL;
    }

    return TP_ERR_OK;
}


/**
 * Compute the angle around a circular segment from the total progress along
 * the curve.
 */
int pmCircleAngleFromProgress(PmCircle const * const circle,
        SpiralArcLengthFit const * const fit,
        double progress,
        double * const angle)
{
    double h2;
    pmCartMagSq(&circle->rHelix, &h2);
    double s_end = pmSqrt(pmSq(fit->total_planar_length) + h2);
    // Parameterize by total progress along helix
    double t = progress / s_end;
    return pmCircleAngleFromParam(circle, fit, t, angle);
}

double spiralEffectiveRadius(PmCircle const * circle)
{
    double dr = circle->spiral / circle->angle;

    // Exact representation of spiral arc length flattened into
    double n_inner = pmSq(dr) + pmSq(circle->radius);
    double den = n_inner+pmSq(dr);
    double num = pmSqrt(pmCb(n_inner));
    double r_spiral = num / den;

    return r_spiral;
}


/**
 * Find the effective minimum radius for acceleration calculations.
 * The radius of curvature of a spiral is larger than the circle of the same
 * radius.
 */
double pmCircleEffectiveMinRadius(PmCircle const * circle)
{
    double h2;
    pmCartMagSq(&circle->rHelix, &h2);
    double dh2 = h2 / pmSq(circle->angle);

    double r_spiral = spiralEffectiveRadius(circle);

    // Curvature of helix, assuming that helical motion is independent of plane motion
    double effective_radius = dh2 / r_spiral + r_spiral;

    return effective_radius;
}

double findTrapezoidalDesiredVel(double a_max,
                                 double dx,
                                 double v_final,
                                 double v_current,
                                 double cycle_time)
{
    double dt = fmax(cycle_time, TP_TIME_EPSILON);
    // Discriminant is 3 terms (when final velocity is non-zero)
    double discr_term1 = pmSq(v_final);
    double discr_term2 = a_max * (2.0 * dx - v_current * dt);
    double tmp_adt = a_max * dt * 0.5;
    double discr_term3 = pmSq(tmp_adt);
    double discr = discr_term1 + discr_term2 + discr_term3;

    //Start with -B/2 portion of quadratic formula
    double maxnewvel = -tmp_adt;

    //If the discriminant term brings our velocity above zero, add it to the total
    //We can ignore the calculation otherwise because negative velocities are clipped to zero
    if (discr > discr_term3) {
        maxnewvel += pmSqrt(discr);
    }

    return maxnewvel;
}

EndCondition checkEndCondition(double cycleTime,
                               double progress,
                               double target,
                               double currentvel,
                               double v_f,
                               double a_max)
{
    double dx = target - progress;
    // Start with safe defaults (segment will not end next cycle
    EndCondition out = {
        v_f,
        TP_BIG_NUM * cycleTime,
    };

    double v_avg = (currentvel + v_f) / 2.0;

    //Check that we have a non-zero "average" velocity between now and the
    //finish. If not, it means that we have to accelerate from a stop, which
    //will take longer than the minimum 2 timesteps that each segment takes, so
    //we're safely far form the end.

    //Get dt assuming that we can magically reach the final velocity at
    //the end of the move.
    //
    //KLUDGE: start with a value below the cutoff
    double dt = TP_TIME_EPSILON / 2.0;
    if (v_avg > TP_VEL_EPSILON) {
        //Get dt from distance and velocity (avoid div by zero)
        dt = fmax(dt, dx / v_avg);
    } else {
        if ( dx > (v_avg * cycleTime) && dx > TP_POS_EPSILON) {
            tc_pdebug_print(" below velocity threshold, assuming far from end\n");
            return out;
        }
    }

    // Assuming a full timestep:
    double dv = v_f - currentvel;
    double a_f = dv / fmax(dt, TP_TIME_EPSILON);

    //If this is a valid acceleration, then we're done. If not, then we solve
    //for v_f and dt given the max acceleration allowed.
    //If we exceed the maximum acceleration, then the dt estimate is too small.
    double a = a_f;
    int recalc = sat_inplace(&a, a_max);

    //Need to recalculate vf and above
    if (recalc) {
        double disc = pmSq(currentvel / a) + 2.0 / a * dx;
        if (disc < 0) {
            //Should mean that dx is too big, i.e. we're not close enough
            tc_pdebug_print(" dx = %f, too large, not at end yet\n",dx);
            return out;
        }

        if (disc < TP_TIME_EPSILON * TP_TIME_EPSILON) {
            tc_pdebug_print("disc too small, skipping sqrt\n");
            dt =  -currentvel / a;
        } else if (a > 0) {
            tc_pdebug_print("using positive sqrt\n");
            dt = -currentvel / a + pmSqrt(disc);
        } else {
            tc_pdebug_print("using negative sqrt\n");
            dt = -currentvel / a - pmSqrt(disc);
        }

        tc_pdebug_print(" revised dt = %f\n", dt);
        //Update final velocity with actual result
        out.v_f = currentvel + dt * a;
    }

    out.dt = dt;
    return out;
}

double pseudo_sqrt(double x)
{
    const double sqrt_c = sqrt(PSEUDO_SQRT_EPSILON);
    const double den = (sqrt(PSEUDO_SQRT_EPSILON + 1) - sqrt_c);
    const double b0 = -sqrt_c;
    const double b1 = 1;
    return (b1 * pmSqrt(x + PSEUDO_SQRT_EPSILON) + b0) / den;
}

/**
 * Force a value to be within the range specified, with that range shrunk by the relative margin.
 * Example: If rel_margin is 0.1, upper = 2, lower = 1, value must be within [1.05,1.95]
 * @pre result is undefined if rel_margin exceeds [0,1], or lower > upper
 */
double clamp(double upper, double lower, double rel_margin, double value)
{
    double m = (upper - lower) * rel_margin / 2.0;
    double u = upper - m;
    double l = lower + m;
    return fmin(fmax(value, l), u);
}

tp_err_t find_blend_points_and_tangents(
    double Rb,
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    blend_boundary_t * const out)
{
    out->s1 = prev_tc->target - Rb;
    out->s2 = Rb;

    //TODO check errs
    EmcPose P1, P2;
    tcGetPosReal(prev_tc, out->s1, &P1);
    tcGetPosReal(tc, out->s2, &P2);

    tcGetTangentUnitVector(prev_tc, out->s1, &out->u1);
    tcGetTangentUnitVector(tc, out->s2, &out->u2);
    out->P1 = P1.tran;
    out->P2 = P2.tran;

    return TP_ERR_OK;
}

int find_blend_intermediate_segments(
    blend_boundary_t const * const blend_params,
    biarc_control_points_t * const control_pts)
{
    // A coefficient
    PmCartesian du12;
    pmCartCartAdd(&blend_params->u1, &blend_params->u2, &du12);

    PmCartesian dP12;
    pmCartCartSub(&blend_params->P1, &blend_params->P2, &dP12);

    // Find B / C first since A could be zero if u1 / u2 are colinear
    double B;
    pmCartCartDot(&dP12, &du12, &B);
    B *= 2.;

    double C;
    pmCartMagSq(&dP12, &C);

    double A;
    pmCartMagSq(&du12, &A);
    A -= 4;

    // true iff u1 / u2 are nearly parallel or anti-parallel
    if (fabs(A) < 1e-12)
    {
        if (fabs(B) < TP_POS_EPSILON) {
            // u vectors are perpendicular to displacement vector between P1/P2
            double dot;
            pmCartCartDot(&blend_params->u1, &blend_params->u2, &dot);
            if (dot > 0) {
                // u1 and -u2 are in opposite directions, no biarc solution exists
                return -2;
            } else {
                // u1 and -u2 are in the same direction, assume "rectangular" solution
                control_pts->d = pmSqrt(C)/2.0;
            }
        } else {
            control_pts->d = -C/B;
        }
    } else {
        double other;
        int res_qf = quadraticFormula(A, B, C, &control_pts->d, &other);
        if (res_qf || control_pts->d < 0) {
            return -1;
        }
    }

    pmCartScalMult(&blend_params->u1, control_pts->d, &control_pts->Pb1);
    pmCartCartAddEq(&control_pts->Pb1, &blend_params->P1);

    pmCartScalMult(&blend_params->u2, -control_pts->d, &control_pts->Pb2);
    pmCartCartAddEq(&control_pts->Pb2, &blend_params->P2);

    pmCartCartDirection(&control_pts->Pb2, &control_pts->Pb1, &control_pts->u_mid);
    pmCartScalMult(&control_pts->u_mid, control_pts->d, &control_pts->P_mid);
    pmCartCartAddEq(&control_pts->P_mid, &control_pts->Pb1);

    if (control_pts->d > 0) {
        return 0;
    } else {
        return -3;
    }
}

static inline double find_max_blend_length(TC_STRUCT const * const tc)
{
    if (tc->motion_type == TC_CIRCULAR) {
        // FIXME use arc length fit for spiral?
        double phi_max = PM_PI / 3.0;
        double h2=0;
        pmCartMagSq(&tc->coords.circle.xyz.rHelix, &h2);
        double total_arc_length = pmSqrt(h2 + pmSq(tc->coords.circle.fit.total_planar_length));
        double l_from_angle = phi_max / tc->coords.circle.xyz.angle * total_arc_length;
        double l_from_length = fmin(tc->target, tc->nominal_length / 3.0);
        return fmin(l_from_angle, l_from_length);
    } else {
        return fmin(tc->target, tc->nominal_length/2.);
    }
}

double find_max_blend_region(TC_STRUCT const * const prev_tc, TC_STRUCT const * const tc)
{
    double l_prev = find_max_blend_length(prev_tc);
    double l_this = find_max_blend_length(tc);
    return fmin(l_prev, l_this);
}

tp_err_t find_blend_size_from_intermediates(
    blend_boundary_t const * const blend_boundary,
    biarc_control_points_t const * const intermediates,
    double *R_geom,
    double *arc_len_est)
{
    double dot;
    pmCartCartDot(&blend_boundary->u1, &intermediates->u_mid, &dot);

    double cos_2theta = fmax(-dot, -1.0 + 1.0e-15);

    double sin_theta_approx = pmSqrt((1. - cos_2theta) / 2);
    double cos_theta_approx = pmSqrt((1. + cos_2theta) / 2);
    double tan_theta_approx = sin_theta_approx / fmax(cos_theta_approx, TP_ANGLE_EPSILON);

    // TODO optimize out the tangent calculation if necessary
    // Find radius from the limiting length (assume length is actually symmetrical
    *R_geom = tan_theta_approx * intermediates->d;
    *arc_len_est = 2.0 * intermediates->d * sin_theta_approx;

    return TP_ERR_OK;
}

static const biarc_solution_t zero_region_result = {0,0,0};

static inline double get_effective_range(blend_solver_constraints_t const * const constraints)
{
    return constraints->upper.Rb - constraints->lower.Rb;
}

static inline double guess_interpolated_region_size(
    blend_solver_constraints_t const * const constraints,
    double R_goal,
    double T_goal)
{
    double dRb = constraints->upper.Rb - constraints->lower.Rb;
    double dR_plan = constraints->upper.R_plan - constraints->lower.R_plan;
    double dR_goal = R_goal - constraints->lower.R_plan;
    double Rb_interp_R = constraints->lower.Rb + dRb * (dR_goal / fmax(dR_plan, TP_POS_EPSILON));

    double dT_plan = constraints->upper.T_plan - constraints->lower.T_plan;
    double dT_goal = T_goal - constraints->lower.T_plan;
    double Rb_interp_T = constraints->lower.Rb + dRb * (dT_goal / fmax(dT_plan, TP_POS_EPSILON));

    if (abs(constraints->bias_count) > 3 ) {
        return (constraints->upper.Rb + constraints->lower.Rb) / 2.;
    } else {
        return clamp(constraints->upper.Rb, constraints->lower.Rb, 0.1, fmin(Rb_interp_R, Rb_interp_T));
    }
}


static inline double guess_blend_region_size_slope(
    blend_solver_constraints_t * const constraints,
    biarc_solution_t const * const solution,
    double R_goal,
    double T_goal)
{
    if (solution->R_plan < R_goal && solution->T_plan < T_goal) {
        constraints->lower = *solution;
        constraints->bias_count = constraints->bias_count > 0 ? -1 : constraints->bias_count - 1;
    } else {
        constraints->bias_count = constraints->bias_count < 0 ? 1 : constraints->bias_count + 1;
        constraints->upper = *solution;
    }

    return guess_interpolated_region_size(constraints, R_goal, T_goal);
}

/**
 * Given an overall tolerance limit and the tangent vectors at the
 * intersection, find an approximate blend region size that puts the midpoint
 * of the intermediate line within tolerance of the intersection point.
 *
 * @note this assumes that the areas to blend are straight-ish (i.e. the error
 * introduced by circular segments is minimal). This assumes a linear
 * approximation about the intersection point, but better approximations are
 * possible.
 */
double find_blend_region_from_tolerance(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double tolerance)
{
    PmCartesian u_tan1, u_tan2;
    // Start by assuming that the blend tangents will be at least 1 tolereance
    // distance away (should always be true because part of the blend will move
    // along the tangential direction until it hits the intermediate segment).
    double s1 = fmax(0, prev_tc->target - tolerance);
    double s2 = fmin(tc->target / 2.0, tolerance);
    tcGetTangentUnitVector(prev_tc, s1, &u_tan1);
    tcGetTangentUnitVector(tc, s2, &u_tan2);

    double eta1 = 0, eta2 = 0;
    if (prev_tc->motion_type == TC_CIRCULAR) {
        eta1 = (prev_tc->target - s1) / prev_tc->coords.circle.xyz.radius;
    }
    if (tc->motion_type == TC_CIRCULAR) {
        eta2 = s2 / tc->coords.circle.xyz.radius;
    }

    double theta = findIntersectionAngle(&u_tan1, &u_tan2);

    // TODO optimize
    double eta = (eta1 + eta2) / 2.0;
    const double theta2_guess = theta + eta;
    double F = 0, theta2 = 0;
    if (theta2_guess < M_PI_2) {
        // "Normal" case where the effective tangent vectors point towards the intersection
        theta2 = theta2_guess;
        F = (sin(eta) + cos(theta))/(sin(theta2) + 1);
    } else {
        // Flipped case, where tangent vectors point away from the intersection
        // If theta2 > pi/2, then the tolerance region "flips" so that the triangles are opposite
        // Redefine theta2 to have the same sense as theta in the flipped triangle
        theta2 = M_PI - theta2_guess;

        F = sin(theta) / (1 + sin(theta2)) * cos(theta2) + cos(theta);
    }
    double Rb = tolerance / F;
    double Rb_max = sin(theta)/sin(theta2) * Rb;

    return Rb_max;
}

double find_blend_region_from_tolerance_simple(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double tolerance)
{
    PmCartesian u_tan1, u_tan2;
    // Start by assuming that the blend tangents will be at least 1 tolereance
    // distance away (should always be true because part of the blend will move
    // along the tangential direction until it hits the intermediate segment).
    tcGetEndTangentUnitVector(prev_tc,  &u_tan1);
    tcGetStartTangentUnitVector(tc, &u_tan2);

    double dot;
    pmCartCartDot(&u_tan1, &u_tan2, &dot);
    double cos_2theta = -dot;

    // avoids acos, sin, cos, but not sure if this is actually faster
    double sin_theta = pmSqrt((1. - cos_2theta) / 2);
    double cos_theta = pmSqrt((1. + cos_2theta) / 2);

    double d_est = sin_theta / cos_theta * tolerance;

    return tolerance / cos_theta + d_est;
}

int find_blend_vel_accel_planar_limits(
    PmCartesian const * const u_tan1,
    PmCartesian const * const u_tan2,
    PmCartesian const * const acc_bound,
    PmCartesian const * const vel_bound,
    double *a_max_planar,
    double *v_max_planar)
{
    // Find an orthonormal basis and "envelope" vector for the plane that represents
    // the worst-case contribution of each axis to the overall velocity / acceleration
    PmCartesian u, v, uv_plane_envelope_sq;
    pmCartCartOrthonormal(u_tan1, u_tan2, &u, &v);
    pmCartCartHarmonicAddSq(&u, &v, &uv_plane_envelope_sq);

    // Reuse the envelope vector (this is why accel and vel are calculated together)
    int res_dia = findMaxValueOnPlane(&uv_plane_envelope_sq, acc_bound, a_max_planar);
    res_dia |= findMaxValueOnPlane(&uv_plane_envelope_sq, vel_bound, v_max_planar);

    return res_dia;
}

void find_biarc_solution_error(
    double blend_tolerance,
    double R_goal,
    double R_plan,
    double T_plan,
    biarc_solution_errors_t * const err)
{
    err->deviation = blend_tolerance - T_plan;

    // Check for solution convergence based on specified tolerances
    err->radius_rel = (R_plan - R_goal + TP_POS_EPSILON) / (R_goal + TP_POS_EPSILON);
    err->radius_abs = R_plan - R_goal;
}

/**
 * Find the optimal Rb to get the smallest blend that can meet the feed rate requirements.
 * Choose initial Rb = Rb_max
 * Find blend start / end points and tangent vectors, within Rb of intersection P with procedure find_blend_points_and_tangents
 * Find blend segment lengths d, L and intermediate points with procedure find_blend_intermediate_segments
 * Calculate blend kinematics for the two blends.
 * Find blend radii and arc lengths given P1, P2, Pb1, Pb2
 * Compute max velocity on blend arcs from geometry
 * Check blend performance:
 * Verify distance from blend midpoint to original intersection P < tolerance
 * Check if requested velocity is reached (if not, need bigger arc)
 * Check if blend radii are larger than they need to be (e.g. worst-case normal acceleration < a_t_max)
 * Refine solution as necessary:
 * Guess a new Rb based on heuristic function of Rb, Rb_max, v_plan with procedure guess_blend_region_size
 * Stop after N iterations or until Rb converges
 */
tp_err_t optimize_biarc_blend_size(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    biarc_solver_config_t const * const config,
    PmCartesian const * const vel_bound,
    PmCartesian const * const acc_bound,
    biarc_solver_results_t * const biarc_results,
    BlendParameters * const param,
    double cycle_time)
{
    if (!prev_tc || !tc || !biarc_results) {
        return TP_ERR_MISSING_INPUT;
    }

    // Find net tolerance that's actually usable (so that we're
    // not passing 0.0 around if tolerance is not specified)
    tcFindBlendTolerance(prev_tc, tc, &param->tolerance);

    //double Rb_init = find_blend_region_from_tolerance(prev_tc, tc, param->tolerance);
    double Rb_init = find_max_blend_region(prev_tc, tc);

    // Initial solver values for Rb range using somewhat dumb (but cheap) values
    biarc_results->solution.Rb = biarc_results->constraints.upper.Rb = Rb_init;
    biarc_results->constraints.upper.R_plan = 0; // To be filled in at first pass through solver
    biarc_results->constraints.lower = zero_region_result;

    const unsigned MAX_ITERATIONS = config->max_iterations;

    blendParamInitVelocities(prev_tc, tc, config->feed_override_allowance, param);

    double s1_max = prev_tc->target - biarc_results->constraints.upper.Rb;
    double s2_max = biarc_results->constraints.upper.Rb;
    PmCartesian u1, u2;
    tcGetTangentUnitVector(prev_tc, s1_max, &u1);
    tcGetTangentUnitVector(tc, s2_max, &u2);

    findVMaxByAltitude(
        &u1,
        &u2,
        param);
    // Approximate the blend kinematics using the initial guess
    // Actual limits will be enforced later
    CHP(blendParamKinematics(
        &u1,
        &u2,
        param,
        acc_bound,
        vel_bound));

    double R_goal = pmSq(param->v_goal) / param->a_n_max;
    // Scale up solver tolerance to have the same net effect on velocity
    const double solver_radius_rel_tol = config->velocity_rel_tolerance * R_goal / param->v_goal;
    const double solver_radius_abs_tol = pmSq(config->velocity_abs_tolerance) / param->a_n_max; // 2% is pretty good

    // Intersection is at start of next segment, so use startpoint method since it's much faster
    EmcPose P;
    CHP(tcGetStartpoint(tc, &P));

    for (biarc_results->iterations = 1; biarc_results->iterations < MAX_ITERATIONS; ++biarc_results->iterations)
    {
        // Find blend start / end points and tangent vectors, within Rb of intersection P with procedure find_blend_points_and_tangents
        find_blend_points_and_tangents(biarc_results->solution.Rb, prev_tc, tc, &biarc_results->solution.boundary);
        // Find blend segment lengths d, L and intermediate points with procedure find_blend_intermediate_segments
        if (TP_ERR_OK != find_blend_intermediate_segments(&biarc_results->solution.boundary, &biarc_results->solution.control_pts))
        {
            biarc_results->result = BIARC_DEGENERATE_SEGMENTS;
            return TP_ERR_FAIL;
        }

        // Verify distance from blend midpoint to original intersection P < tolerance
        pmCartCartDisp(&biarc_results->solution.control_pts.P_mid, &P.tran, &biarc_results->solution.T_plan);

        CHP(find_blend_size_from_intermediates(
            &biarc_results->solution.boundary,
            &biarc_results->solution.control_pts,
            &biarc_results->solution.R_geom,
            &biarc_results->solution.arc_len_est
            ));

        // Cheat by expressing max radius in terms of v_max even though it's not strictly the limiting factor
        double guess_v_max_length = biarc_results->solution.arc_len_est / cycle_time;
        double guess_R_effective_max = pmSq(guess_v_max_length) / param->a_n_max;

        // THis clamping throws off the slope-based update. Need to do slopes based on R_geom, or somehow compensate for it
        biarc_results->solution.R_plan = fmin(biarc_results->solution.R_geom, guess_R_effective_max);
        // Check for solution convergence based on specified tolerances
        find_biarc_solution_error(
            param->tolerance,
            R_goal,
            biarc_results->solution.R_plan,
            biarc_results->solution.T_plan,
            &biarc_results->solution.err);

        // Need an ok tolerance or we're not done
        if (biarc_results->solution.err.deviation >= -config->deviation_abs_tolerance) {
            // Check if we're close to the goal "radius", i.e. we've met the blend target velocity limit
            if ((fabs(biarc_results->solution.err.radius_rel) < solver_radius_rel_tol
                 || fabs(biarc_results->solution.err.radius_abs) < solver_radius_abs_tol))
            {
                biarc_results->result = BIARC_REACHED_V_GOAL;
                return TP_ERR_OK;
            } else if (biarc_results->solution.err.radius_rel < 0.0 && biarc_results->solution.err.deviation < config->deviation_abs_tolerance) {
                biarc_results->result = BIARC_REACHED_TOLERANCE;
                return TP_ERR_OK;
            } else if (get_effective_range(&biarc_results->constraints) < config->convergence_tolerance) {
                biarc_results->result = BIARC_CONVERGED;
                return TP_ERR_OK;
            }
        } else {
            double R_bound = fmax(biarc_results->constraints.upper.R_plan, biarc_results->constraints.lower.R_plan);
            double v_bound = pmSqrt(R_bound * param->a_n_max);
            if (biarc_results->iterations > 1 && v_bound < config->velocity_cutoff) {
                biarc_results->result = BIARC_MIN_LENGTH_LIMITED;
                return TP_ERR_FAIL;
            }
        }
        // Refine Rb if solution has not converged
        biarc_results->solution.Rb = guess_blend_region_size_slope(
            &biarc_results->constraints,
            &biarc_results->solution,
            R_goal,
            param->tolerance);
    }

    // Ran out of iterations, try the best solution we have

    biarc_results->solution = biarc_results->constraints.lower;

    if (biarc_results->solution.err.deviation >= -config->deviation_abs_tolerance) {
        biarc_results->result = BIARC_EXCEEDED_MAX_ITERATIONS;
        return TP_ERR_OK;
    } else {
        biarc_results->result = BIARC_FAILED_TO_CONVERGE;
        return TP_ERR_FAIL;
    }
}

#ifdef UNIT_TEST
#include <stdio.h>
#endif
#ifdef DO_BIARC_SCAN
tp_err_t scan_blend_properties(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    biarc_solver_config_t const * const config,
    PmCartesian const * const vel_bound,
    PmCartesian const * const acc_bound,
    biarc_solver_results_t * const biarc_results,
    BlendParameters * const param,
    double cycle_time,
    double resolution
    )
{
    if (!prev_tc || !tc || !biarc_results) {
        return TP_ERR_MISSING_INPUT;
    }

    // Find net tolerance that's actually usable (so that we're
    // not passing 0.0 around if tolerance is not specified)
    tcFindBlendTolerance(prev_tc, tc, &param->tolerance);

    blend_solver_constraints_t constraints;

    // Initial solver values for Rb range using somewhat dumb (but cheap) values
    constraints.upper = find_max_blend_region(prev_tc, tc);
    constraints.tolerance_upper = constraints.upper;

    blendParamInitVelocities(prev_tc, tc, config->feed_override_allowance, param);

    double s1_max = prev_tc->target - constraints.tolerance_upper;
    double s2_max = constraints.tolerance_upper;
    PmCartesian u1, u2;
    tcGetTangentUnitVector(prev_tc, s1_max, &u1);
    tcGetTangentUnitVector(tc, s2_max, &u2);

    findVMaxByAltitude(
        &u1,
        &u2,
        param);
    //TODO do altitude calculation too?
    // Approximate the blend kinematics using the initial guess
    // Actual limits will be enforced later
    CHP(blendParamKinematics(
        &u1,
        &u2,
        param,
        acc_bound,
        vel_bound));

    // Intersection is at start of next segment, so use startpoint method since it's much faster
    EmcPose P;
    CHP(tcGetStartpoint(tc, &P));

#ifdef UNIT_TEST
    printf(">a_n_max %f\n", param->a_n_max);
    printf("|f,%12s,%12s,%12s,%12s,%12s,%12s\n",
           "Rb",
           "tol_dev",
           "R_geom",
           "R_plan",
           "v_plan",
           "d");
#endif
    const unsigned MAX_ITERATIONS = (unsigned)((double)constraints.upper.Rb / resolution);
    for (biarc_results->iterations = 1; biarc_results->iterations < MAX_ITERATIONS; ++biarc_results->iterations)
    {
        biarc_results->Rb = (double)biarc_results->iterations * resolution;
        // Find blend start / end points and tangent vectors, within Rb of intersection P with procedure find_blend_points_and_tangents
        find_blend_points_and_tangents(biarc_results->Rb, prev_tc, tc, &biarc_results->boundary);
        // Find blend segment lengths d, L and intermediate points with procedure find_blend_intermediate_segments
        int failed = 0;
        double T;
        if (TP_ERR_OK != find_blend_intermediate_segments(&biarc_results->boundary, &biarc_results->control_pts))
        {
            failed = 1;
            T=-1;
        } else {

        // Verify distance from blend midpoint to original intersection P < tolerance
        pmCartCartDisp(&biarc_results->control_pts.P_mid, &P.tran, &T);

        CHP(find_blend_size_from_intermediates(
            &biarc_results->boundary,
            &biarc_results->control_pts,
            &biarc_results->R_geom,
            &biarc_results->arc_len_est));

        // Cheat by expressing max radius in terms of v_max even though it's not strictly the limiting factor
        double guess_v_max_length = biarc_results->arc_len_est / cycle_time;
        double guess_R_effective_max = pmSq(guess_v_max_length) / param->a_n_max;

        biarc_results->R_plan = fmin(biarc_results->R_geom, guess_R_effective_max);
        }
#ifdef UNIT_TEST
            printf("|%d,%12f,%12f,%12f,%12f,%12f,%12f\n",
               failed,
               biarc_results->Rb,
               T,
               biarc_results->R_geom,
               biarc_results->R_plan,
               pmSqrt(param->a_n_max * biarc_results->R_plan),
               biarc_results->control_pts.d);
#endif
    }
    biarc_results->result = BIARC_EXCEEDED_MAX_ITERATIONS;
    return TP_ERR_FAIL;
}
#endif

const char * biarc_result_to_str(BiarcSolverResults r)
{
    switch (r) {
    case BIARC_DEGENERATE_SEGMENTS:
        return "degenerate";
    case BIARC_FAILED_TO_CONVERGE:
        return "diverged";
    case BIARC_EXCEEDED_MAX_ITERATIONS:
        return "max_iterations";
    case BIARC_NOT_SOLVED:
        return "not_solved";
    case BIARC_REACHED_V_GOAL:
        return "reached_v_goal";
    case BIARC_REACHED_TOLERANCE:
        return "reached_tol";
    case BIARC_CONVERGED:
        return "converged";
    case BIARC_MIN_LENGTH_LIMITED:
        return "hit_min_len";
    }
    return "";
}

tp_err_t blend_biarc_postprocess(
    biarc_solver_results_t const * const biarc_results,
    BlendPoints3 * const points1,
    BlendPoints3 * const points2)
{
    {
        double theta1 = findIntersectionAngle(
            &biarc_results->solution.boundary.u1,
            &biarc_results->solution.control_pts.u_mid);

        if (M_PI_2 - theta1 < TP_ANGLE_EPSILON) {
            points1->motion_type = TC_LINEAR;
        } else {
            points1->motion_type = TC_SPHERICAL;
            PmCartesian normal1;
            pmCartCartSub(
                &biarc_results->solution.control_pts.u_mid,
                &biarc_results->solution.boundary.u1,
                &normal1);
            pmCartUnitEq(&normal1);

            double center_dist1 = biarc_results->solution.control_pts.d / cos(theta1);

            pmCartScalMult(&normal1, center_dist1, &points1->arc_center);
            pmCartCartAddEq(&points1->arc_center, &biarc_results->solution.control_pts.Pb1);
        }

        points1->arc_start = biarc_results->solution.boundary.P1;
        points1->arc_end = biarc_results->solution.control_pts.P_mid;
    }
    {
        double theta2 = findIntersectionAngle(
            &biarc_results->solution.control_pts.u_mid,
            &biarc_results->solution.boundary.u2);

        if (M_PI_2 - theta2 < TP_ANGLE_EPSILON) {
            points2->motion_type = TC_LINEAR;
        } else {
            points2->motion_type = TC_SPHERICAL;
            PmCartesian normal2;
            pmCartCartSub(
                &biarc_results->solution.boundary.u2,
                &biarc_results->solution.control_pts.u_mid,
                &normal2);
            pmCartUnitEq(&normal2);

            double center_dist2 = biarc_results->solution.control_pts.d / cos(theta2);

            pmCartScalMult(&normal2, center_dist2, &points2->arc_center);
            pmCartCartAddEq(&points2->arc_center, &biarc_results->solution.control_pts.Pb2);
        }
        points2->arc_start = biarc_results->solution.control_pts.P_mid;
        points2->arc_end = biarc_results->solution.boundary.P2;
    }
    return TP_ERR_OK;
}
