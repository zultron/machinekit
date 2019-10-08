/********************************************************************
* Description: blendmath.h
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
#ifndef BLENDMATH_H
#define BLENDMATH_H

#include "posemath.h"
#include "tc_types.h"
#include "tp_enums.h"

#define BLEND_ACC_RATIO_TANGENTIAL 0.5
#define BLEND_ACC_RATIO_NORMAL (pmSqrt(1.0 - pmSq(BLEND_ACC_RATIO_TANGENTIAL)))
#define BLEND_KINK_FACTOR 0.25

/**
 * 3D Input geometry for a spherical blend arc. 
 * This structure contains all of the basic geometry in 3D for a blend arc.
 */
typedef struct {
    PmCartesian u1;         /* Tangent unit vector at intersection point for segment 1 / 2*/
    PmCartesian u2;
    PmCartesian P;          /* Intersection point */
    PmCartesian normal;     /* normal unit vector to plane containing lines */
    double theta;
} BlendGeom3;


/**
 * Blend arc parameters (abstracted).
 * This structure holds blend arc parameters that have been abstracted from the
 * physical geometry. This data is used to find the maximum radius given the
 * constraints on the blend. By abstracting the parameters from the geometry,
 * the same calculations can be used with any input geometry (lines, arcs, 6 or
 * 9 dimensional lines). 
 */
typedef struct {
    double v_max_geom1;      /* ini_maxvel for each segment */
    double v_max_geom2;
    double v_req;       /* requsted velocity for the blend arc */
    double v_goal;      /* desired velocity at max feed override */

    double tolerance;   /* Net blend tolerance (min of line 1 and 2) */

    double L1;          /* Available part of line 1 to blend over */
    double L2;          /* Available part of line 2 to blend over */
} BlendControls;

typedef struct {
    /* These fields are considered "output", and may be refactored into a
     * separate structure in the future */

    double theta;       /* Intersection angle, half of angle between -u1 and u2 */
    double phi;         /* supplement of intersection angle, angle between u1 and u2 */
    double a_n_max;     /* max normal acceleration allowed */
    double a_max_planar;       /* max acceleration allowed for blend */

    double R_plan;      /* planned radius for blend arc */
    double d_plan;      /* distance along each line to arc endpoints */

    double v_max_planar; /* maximum possible velocity in blend plane */
    double v_max_altitude;      /* overall maximum possible velocity during blend segment */
    double v_plan;      /* planned max velocity at max feed override */
    double v_actual;    /* velocity at feedscale = 1.0 */
    double s_arc;       /* arc length */
    int is_consumable;        /* Consume the previous segment */
    int consume;        /* Consume the previous segment */
    double line_length;
} BlendParameters;

/**
 * Output geometry in 3D.
 * Stores the three points representing a simple 3D spherical arc.
 */
typedef struct {
    PmCartesian arc_start;      /* start point for blend arc */
    PmCartesian arc_end;        /* end point for blend arc */
    PmCartesian arc_center;     /* center point for blend arc */
    PmCartesian u_tan;
    tc_motion_type_t motion_type;
} BlendPoints3;

double findMaxTangentAngle(double v, double acc, double cycle_time);

double findKinkAccel(double kink_angle, double v_plan, double cycle_time);

double findIntersectionAngle(PmCartesian const * const u1,
        PmCartesian const * const u2);

double findIntersectionTanAngle(PmCartesian const * const u1,
    PmCartesian const * const u2);

double pmCartMin(PmCartesian const * const in);

int calculateInscribedRadius(PmCartesian const * const normal,
        PmCartesian const * const bounds, double * const radius);

int findAccelScale(PmCartesian const * const acc,
        PmCartesian const * const bounds,
        PmCartesian * const scale);

int findMaxValueOnPlane(PmCartesian const * plane_envelope_sq,
        PmCartesian const * bounds,
        double * max_planar_value);

int quadraticFormula(
    double A,
    double B,
    double C,
    double * const root0,
    double * const root1);

double findTrapezoidalDesiredVel(double a_max,
                              double dx,
                              double v_final,
                              double currentvel,
                              double cycle_time);

int pmCartCartParallel(PmCartesian const * const u1,
        PmCartesian const * const u2,
        double tol);

int pmCartCartAntiParallel(PmCartesian const * const u1,
        PmCartesian const * const u2,
        double tol);

int pmCircLineCoplanar(PmCircle const * const circ,
        PmCartLine const * const line, double tol);

int blendCoplanarCheck(PmCartesian const * const normal,
        PmCartesian const * const u1_tan,
        PmCartesian const * const u2_tan,
        double tol);

int blendCalculateNormals3(BlendGeom3 * const geom);

int blendComputeParameters(BlendParameters * const param, const BlendControls * const controls);

int blendCheckConsume(BlendParameters * const param,
        double L_prev,
        TC_STRUCT const * const prev_tc, int gap_cycles);

int blendFindPoints3(BlendPoints3 * const points, BlendGeom3 const * const geom,
        BlendParameters const * const param);

int blendGeom3Init(BlendGeom3 * const geom,
        TC_STRUCT const * const prev_tc,
        TC_STRUCT const * const tc);

int find_blend_parameters(
    const PmCartesian * const u_tan1,
    const PmCartesian * const u_tan2,
    PmCartesian const * const acc_bound,
    PmCartesian const * const vel_bound,
    const BlendControls * const controls,
    BlendParameters * const param);

int blendInitControls(
    BlendControls * const controls,
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double maxFeedScale);

int blendInit3FromLineLine(
    BlendGeom3 * const geom,
    BlendParameters * const param,
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc);

int blendGeom3Print(BlendGeom3 const * const geom);
int blendParamPrint(BlendParameters const * const param);
int blendPoints3Print(BlendPoints3 const * const points);

typedef struct {
    double v_max;
    double acc_ratio;
} PmCircleLimits;

PmCircleLimits pmCircleActualMaxVel(const PmCircle *circle,
        double v_max_nominal,
        double a_max_nominal);

int findSpiralArcLengthFit(PmCircle const * const circle,
        SpiralArcLengthFit * const fit);
int pmCircleAngleFromProgress(PmCircle const * const circle,
        SpiralArcLengthFit const * const fit,
        double progress,
        double * const angle);
double pmCircleEffectiveMinRadius(const PmCircle *circle);
double spiralEffectiveRadius(PmCircle const * circle);

static inline double findVPeak(double a_t_max, double distance)
{
    return pmSqrt(a_t_max * distance);
}

// Start by mimicing the current structure for minimal changes
typedef struct {
    double v_f;
    double dt;
} EndCondition;

EndCondition checkEndCondition(double cycleTime,
                               double progress,
                               double target,
                               double currentvel,
                               double v_f,
                               double a_max);

#define PSEUDO_SQRT_EPSILON 0.001
double pseudo_sqrt(double x);

int find_blend_vel_accel_planar_limits(
    PmCartesian const * const u_tan1,
    PmCartesian const * const u_tan2,
    PmCartesian const * const acc_bound,
    PmCartesian const * const vel_bound,
    double *a_max_planar,
    double *v_max_planar);

void findVMaxByAltitude(PmCartesian const * const u1,
    PmCartesian const * const u2, const BlendControls * const controls,
    BlendParameters * const param);

int blendParamInitVelocities(TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double override_allowance,
    BlendControls * const controls);

tp_err_t init_blend_segment_from_points(
    TC_STRUCT * const blend_tc,
    BlendPoints3 const *points,
    BlendParameters * const param);

// BiArc blend types
typedef struct
{
    PmCartesian P1;
    PmCartesian u1;
    PmCartesian P2;
    PmCartesian u2;
    double s1;
    double s2;
} blend_boundary_t;

typedef struct
{
    PmCartesian Pb1;
    PmCartesian Pb2;
    PmCartesian u_mid;
    PmCartesian P_mid;
    double d; //!< Length of intermediate blend segment
} biarc_control_points_t;


typedef enum
{
    BIARC_DEGENERATE_SEGMENTS=-3,
    BIARC_FAILED_TO_CONVERGE=-2,
    BIARC_EXCEEDED_MAX_ITERATIONS=-1,
    BIARC_NOT_SOLVED,
    BIARC_REACHED_V_GOAL,
    BIARC_REACHED_TOLERANCE,
    BIARC_CONVERGED,
    BIARC_MIN_LENGTH_LIMITED,
} BiarcSolverResults;

typedef struct
{
    double deviation;
    double radius_rel;
    double radius_abs;
} biarc_solution_errors_t;

typedef struct {
    double Rb; //!< size of blend region (i.e. distance to go along motion segments from intersection to blend boundary points)
    double R_geom; //!< Estimated blend radius based on geometry
    double arc_len_est; //!< Estimated arc length of first blend
    double R_plan; //!< Effective blend "radius" for a max blend velocity, based on both arc length and geometry .
    double T_plan; //!< Blend tolerance (deviation from intersection point) for Rb
    biarc_solution_errors_t err; //!< error terms from tolerance / velocity goals
    blend_boundary_t boundary; //!< Boundary of the blend solution corresponding to Rb
    biarc_control_points_t control_pts; //!< Control points for the intermediate line segments making up the biarc blend
} biarc_solution_t;

typedef struct
{
    biarc_solution_t upper;
    biarc_solution_t lower;
    int bias_count; //!< KLUDGE track number of times the solution has been on the same side of the upper / lower divide
} blend_solver_constraints_t;

typedef struct
{
    blend_solver_constraints_t constraints; //!< blend solver internals (upper / lower bounds on Rb)
    unsigned iterations; //!< solver iterations
    BiarcSolverResults result;
    biarc_solution_t solution;
} biarc_solver_results_t;

typedef struct
{
    EmcPose p_diff;
    PmCartesian u_diff;
    double dot;
} ContinuityCheck;


// API for biarc blends

const char * biarc_result_to_str(BiarcSolverResults r);

double find_blend_region_from_tolerance(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double tolerance);

double find_blend_region_from_tolerance_simple(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    double tolerance);

tp_err_t find_blend_points_and_tangents(
    double Rb,
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    blend_boundary_t * const out);

int find_blend_intermediate_segments(blend_boundary_t const * const blend_params,
    biarc_control_points_t * const control_pts);

double find_max_blend_region(TC_STRUCT const * const prev_tc, TC_STRUCT const * const tc, double v_goal);

tp_err_t find_blend_size_from_intermediates(blend_boundary_t const * const blend_boundary,
    biarc_control_points_t const * const intermediates,
    double *R_geom, double * arc_len_est);

tp_err_t optimize_biarc_blend_size(TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    const biarc_solver_config_t * const config,
    PmCartesian const * const vel_bound,
    PmCartesian const * const acc_bound,
    biarc_solver_results_t * const biarc_results, BlendControls * const controls,
    BlendParameters * const param,
    double cycle_time);

// For testing only
tp_err_t scan_blend_properties(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const tc,
    biarc_solver_config_t const * const config,
    PmCartesian const * const vel_bound,
    PmCartesian const * const acc_bound,
    biarc_solver_results_t * const biarc_results,
    BlendParameters * const param,
    double cycle_time,
    double resolution);

int blend_find_arcpoints3(
    BlendPoints3 * const points,
    biarc_solver_results_t const * const);

int find_biarc_points_from_solution(PmCartesian const * const u1,
    PmCartesian const * const u2,
    PmCartesian const * const P1,
    PmCartesian const * const P2,
    PmCartesian const * const P,
    double d,
    PmCartesian const * const acc_bound,
    PmCartesian const * const vel_bound, const BlendControls * const controls,
    BlendPoints3 * const points,
    BlendParameters *const param);

ContinuityCheck calc_C1_continuity(
    TC_STRUCT const * const prev_tc,
    TC_STRUCT const * const next_tc);

#endif
