/********************************************************************
 * Description: spherical_arc.c
 *
 * A simple spherical linear interpolation library and related functions.
 *
 * Author: Robert W. Ellenberg
 * License: GPL Version 2
 * System: Linux
 *    
 * Copyright (c) 2014 All rights reserved.
 *
 ********************************************************************/

#include "posemath.h"
#include "spherical_arc.h"
#include "rtapi_math.h"
#include "tp_enums.h"
#include "tp_debug.h"

int arcInitFromPoints(
    SphericalArc * const arc,
    PmCartesian const * const start,
    PmCartesian const * const end,
    PmCartesian const * const center,
    PmCartesian const * const uTan,
    double prev_line_length)
{
#ifdef ARC_PEDANTIC
    if (!start || !end || !center) {
        return TP_ERR_MISSING_INPUT;
    }
    if (!arc) {
        return TP_ERR_MISSING_OUTPUT;
    }
#endif

    arc->uTan = *uTan;
    arc->line_length = prev_line_length;
    if (arc->line_length < 0) {
        return TP_ERR_INVALID;
    }

    // Store the start, end, and center
    arc->start = *start;
    arc->end = *end;
    arc->center = *center;

    pmCartCartSub(start, center, &arc->rStart);
    pmCartCartSub(end, center, &arc->rEnd);

    // Find the radii at start and end. These are identical for a perfect spherical arc
    double radius0, radius1;
    pmCartMag(&arc->rStart, &radius0);
    pmCartMag(&arc->rEnd, &radius1);

    tp_debug_print("radii are %g and %g\n",
            radius0,
            radius1);

    if (radius0 < ARC_MIN_RADIUS || radius1 < ARC_MIN_RADIUS) {
        tp_debug_print("radius below min radius %f, aborting arc\n",
                ARC_MIN_RADIUS);
        return TP_ERR_RADIUS_TOO_SMALL;
    }

    // Choose initial radius as nominal radius
    arc->radius = radius0;

    // Get unit vectors from center to start and center to end
    PmCartesian u0, u1;
    pmCartScalMult(&arc->rStart, 1.0 / radius0, &u0);
    pmCartScalMult(&arc->rEnd, 1.0 / radius1, &u1);

    // Find arc angle
    double dot;
    pmCartCartDot(&u0, &u1, &dot);
    arc->angle = acos(dot);
    tp_debug_print("spherical arc angle = %f\n", arc->angle);

    // Store spiral factor as radial difference. Archimedean spiral coef. a = spiral / angle
    arc->spiral = (radius1 - radius0 );

    if (arc->angle < ARC_MIN_ANGLE) {
        tp_debug_print("angle %f below min angle %f, aborting arc\n",
                arc->angle,
                ARC_MIN_ANGLE);
        return TP_ERR_GEOM;
    }

    // Store sin of arc angle since it is reused many times for SLERP
    arc->Sangle = sin(arc->angle);

    return TP_ERR_OK;
}

int arcPoint(SphericalArc const * const arc, double progress, PmCartesian * const out)
{
#ifdef TP_PEDANTIC
    if (!arc) {return TP_ERR_MISSING_INPUT;}
    if (!out) {return TP_ERR_MISSING_OUTPUT;}
#endif

    //Convert progress to actual progress around the arc
    double net_progress = progress - arc->line_length;
    if (net_progress <= 0.0 && arc->line_length > 0) {
        //Get position on line (not actually an angle in this case)
        pmCartScalMult(&arc->uTan, net_progress, out);
        pmCartCartAdd(out, &arc->start, out);
    } else {
        double angle_in = net_progress / arc->radius;
        double scale0 = sin(arc->angle - angle_in) / arc->Sangle;
        double scale1 = sin(angle_in) / arc->Sangle;

        PmCartesian interp0,interp1;
        pmCartScalMult(&arc->rStart, scale0, &interp0);
        pmCartScalMult(&arc->rEnd, scale1, &interp1);

        pmCartCartAdd(&interp0, &interp1, out);
        pmCartCartAdd(&arc->center, out, out);
    }
    return TP_ERR_OK;
}

double arcLength(SphericalArc const * const arc)
{
    return arc->radius * arc->angle + arc->line_length;
}

/**
 * Tests if a ray from the specified point / direction intersects the circle with the given center.
 *
 * @note that the circle is assumed to be coindicent with P (so the radius is || P - center ||).
 *
 * For blending, this is used to determine if a given segment forms a convex or
 * concave intersection with another segment. "Convex" means that the tangent
 * vector of the other segment (at P) is in the interior of the circle
 * containing this segment. For an arc-arc intersection, there are actually 4 possibilities:
 *
 * convex-convex (each arc's tangent vector is in the other's interior)
 * concave-convex
 * convex-concave
 * concave-concave
 */
int checkRayIntersectsArc(PmCartesian const * const center,
                          PmCartesian const * const P,
                          PmCartesian const * const u_ray,
                          int reverse_dir)
{
    double dot;
    PmCartesian r_CP;
    pmCartCartSub(P, center, &r_CP);
    pmCartCartDot(&r_CP, u_ray, &dot);

    int convex = (reverse_dir != 0) ^ (dot < 0);
    return convex;
}

int arcTangent(SphericalArc const * const arc, double const t, PmCartesian * const out)
{
    if (!arc || !out) {
        return TP_ERR_MISSING_INPUT;
    }

    // This section implements the derivative of the SLERP formula to get a
    // local tangent vector.
    const double theta = arc->angle;
    const double k = theta / arc->Sangle;
    const double k0 = -cos( (1.0 - t) * theta);
    const double k1 = cos( t * theta);

    PmCartesian dp0,dp1;

    // Ugly sequence to build up tangent vector from components of the derivative
    pmCartScalMult(&arc->rStart, k * k0, &dp0);
    pmCartScalMult(&arc->rEnd, k * k1, &dp1);
    pmCartCartAdd(&dp0, &dp1, out);

    // tangential vector complete, now normalize
    pmCartUnitEq(out);

    return TP_ERR_OK;
}
