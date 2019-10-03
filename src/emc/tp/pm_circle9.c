#include "rtapi.h"
#include "pm_circle9.h"
#include "blendmath_types.h"
#include "rtapi_bool.h"
#include "rtapi_math.h"

int pmCircle9Init(
    PmCircle9 * const circ9,
    PmVector const * const start,
    PmVector const * const end,
    PmCartesian const * const center,
    PmCartesian const * const normal,
    int turn)
{
    PmCartesian start_xyz, end_xyz;
    PmCartesian start_uvw, end_uvw;
    PmCartesian start_abc, end_abc;

    VecToCart(start, &start_xyz, &start_abc, &start_uvw);
    VecToCart(end, &end_xyz, &end_abc, &end_uvw);

    int xyz_fail = pmCircleInit(&circ9->xyz, &start_xyz, &end_xyz, center, normal, turn);
    //Initialize line parts of Circle9

    int abcuvw_fail = pmLine9Init(&circ9->abcuvw, start, end, true);

    int res_fit = findSpiralArcLengthFit(&circ9->xyz, &circ9->fit, 1e-6);

    if (xyz_fail || abcuvw_fail || res_fit) {
        rtapi_print_msg(RTAPI_MSG_ERR,"Failed to initialize Circle9, err codes %d, %d, %d\n",
                xyz_fail, abcuvw_fail, res_fit);
        return -1;
    }

    return 0;
}

double pmCircle9Length(PmCircle9 const * const circ9)
{
    double h2;
    pmCartMagSq(&circ9->xyz.rHelix, &h2);
    double abcuvw_length = circ9->abcuvw.tmag;
    return pmSqrt(pmSq(circ9->fit.total_planar_length) + h2 + pmSq(abcuvw_length));
}

int pmCircle9TangentVector(PmCircle9 const * const circ9, double progress, PmVector * const out)
{
    // Scale each portion of the motion by it's total displacement to get the 9D tangent vector
    PmCircle const *circle = &circ9->xyz;

    double h2;
    pmCartMagSq(&circ9->xyz.rHelix, &h2);
    double abcuvw_length = circ9->abcuvw.tmag;
    double helical_length_xyz = pmSqrt(pmSq(circ9->fit.total_planar_length) + h2);
    double total_length = pmSqrt(pmSq(circ9->fit.total_planar_length) + h2 + pmSq(abcuvw_length));

    double progress_xyz = progress / total_length * helical_length_xyz;
    double angle_in=0;
    pmCircleAngleFromProgress(circle, &circ9->fit, progress_xyz, &angle_in);

    PmCartesian xyz;
    pmCircleTangentVector(circle, angle_in, &xyz);
    pmCartScalMultEq(&xyz, helical_length_xyz);

    *out = circ9->abcuvw.uVec;
    VecScalMultEq(out, abcuvw_length);
    VecSetXYZ(&xyz, out);
    VecUnitEq(out);
    return 0;
}

/**
 * Find the geometric tangent vector to a helical arc.
 * Unlike the acceleration vector, the result of this calculation is a vector
 * tangent to the helical arc. This is called by wrapper functions for the case of a circular or helical arc.
 */
int pmCircleTangentVector(
    PmCircle const * const circle,
    double angle_in,
    PmCartesian * const u_tan_out)
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

PmCircleLimits pmCircleActualMaxVel(
    PmCircle const * circle,
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

    PmCircleLimits limits = {
        v_max_actual,
        acc_ratio_tan
    };

    return limits;
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
int findSpiralArcLengthFit(
    PmCircle const * const circle,
    SpiralArcLengthFit * const fit,
    double angle_tolerance)
{
    // Additional data for arc length approximation
    double spiral_coef = circle->spiral / circle->angle;
    double min_radius = circle->radius;

    if (circle->spiral < 0.0) {
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

    // Check against start and end angle
    double angle_end_chk = 0.0;
    int res_angle = pmCircleAngleFromParam(circle, fit, 1.0, &angle_end_chk);
    if (res_angle != 0) {
        //TODO better error message
        rtapi_print_msg(RTAPI_MSG_ERR,
                "Spiral fit failed\n");
        return -1;
    }

    // Check fit against angle
    double fit_err = angle_end_chk - circle->angle;
    if (fabs(fit_err) > angle_tolerance) {
        rtapi_print_msg(RTAPI_MSG_ERR,
                "Spiral fit angle difference is %e, maximum allowed is %e\n",
                fit_err,
                angle_tolerance);
        return -1;
    }

    return 0;
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

/**
 * Intermediate function to find the angle for a parameter from 0..1 along the
 * spiral arc.
 */
int pmCircleAngleFromParam(PmCircle const * const circle,
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
        return -1;
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
    return 0;
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

int pmCircle9Point(PmCircle9 const * const circ9, double progress, PmVector * const point)
{
    //TODO
    return 0;
}

int pmCircle9Stretch(PmCircle9 * const circ, double new_total_len, bool from_end)
{
    //TODO
#if 0

    if (!circ || new_angle <= DOUBLE_FUZZ) {
        return PM_ERR;
    }

    if (from_end) {
        // Need some extra juggling if we're creating a new start point
        PmCartesian new_start;
        double start_angle = circ->angle - new_angle;
        pmCirclePoint(circ, start_angle, &new_start);
        PmCartesian tmp;
        pmCartCartSub(&new_start, &circ->center, &tmp);
        // Need to find new components perpendicular to circle normal, and to each other
        pmCartCartCross(&circ->normal, &tmp, &circ->rPerp);
        pmCartCartCross(&circ->rPerp, &circ->normal, &circ->rTan);
        // Center is displaced if there is a helical component
        pmCartCartSubEq(&tmp, &circ->rTan);
        pmCartCartAddEq(&circ->center, &tmp);
        // New initial radius in case there is non-zero spiral component
        pmCartMag(&circ->rTan, &circ->radius);
    }
    // Reduce the spiral / helical components proportionally
    double angle_ratio = (new_angle / circ->angle);
    circ->spiral *= angle_ratio;
    pmCartScalMultEq(&circ->rHelix, angle_ratio);
    circ->angle = new_angle;
#endif
    return 0;
}

