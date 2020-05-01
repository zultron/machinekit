#include "path_geom_util.hh"
#include "emcglb.h"
#include <numeric>

double hypot3(double a, double b, double c)
{
    return sqrt(a*a + b*b + c*c);
}

/**
 * Get the shortest linear axis displacement that the TP can handle as a discrete move.
 *
 * If this looks dirty, it's because it is. Canon runs in its own units, but
 * the TP uses user units. Therefore, the minimum displacement has to be
 * computed the same way, with the same threshold, or short moves do strange
 * things (accel violations or infinite pauses).
 *
 * @todo revisit this when the TP is overhauled to use a consistent set of internal units.
 */
double getTPMinLinearDisplacement()
{
    return 1e-7;
}

/**
 * Equivalent of getMinLinearDisplacement for rotary axes.
 */
double getTPMinAngularDisplacement()
{
    return 1e-7;
}

static PositionArray const &getMinDisplacement()
{
    const double tiny_linear = getTPMinLinearDisplacement();
    const double tiny_angular = getTPMinAngularDisplacement();
    static PositionArray min_disp = {
        axis_valid(0) ?  tiny_linear : INFINITY,
        axis_valid(1) ?  tiny_linear : INFINITY,
        axis_valid(2) ?  tiny_linear : INFINITY,
        axis_valid(3) ?  tiny_angular : INFINITY,
        axis_valid(4) ?  tiny_angular : INFINITY,
        axis_valid(5) ?  tiny_angular : INFINITY,
        axis_valid(6) ?  tiny_linear : INFINITY,
        axis_valid(7) ?  tiny_linear : INFINITY,
        axis_valid(8) ?  tiny_linear : INFINITY,
    };
    return min_disp;
}

static auto const do_square = [](double total, double p) { return total + (p * p);};

static double getAngularDisplacement(PositionArray const &dp)
{
    return sqrt(std::accumulate(dp.begin()+3, dp.begin()+6, 0.0, do_square));
}

static double getXYZDisplacement(PositionArray const &dp)
{
    return sqrt(std::accumulate(dp.begin(), dp.begin()+3, 0.0, do_square));
}

static double getUVWDisplacement(PositionArray const &dp)
{
    return sqrt(std::accumulate(dp.begin()+6, dp.begin()+9, 0.0, do_square));
}

DisplacementData getStraightDisplacement(PositionArray const &start_pt, PositionArray const &goal_pt)
{
    DisplacementData out{};
    out.displacement = abs(goal_pt - start_pt);
    // TODO decide if this step is necessary?
    apply_abs_mask(out.displacement, getMinDisplacement());

    // NOTE: need to work in external (TP) units here, since the TP assumes
    // that all axes are "path units". 1 path unit = 1 in = 1 deg (for the
    // typical config).
    // TODO replace with to_path_XXX functions when we implement optimized path units
    out.d_xyz = getXYZDisplacement(out.displacement);
    out.d_abc = getAngularDisplacement(out.displacement);
    out.d_uvw = getUVWDisplacement(out.displacement);
    out.d_tot = hypot3(out.d_xyz, out.d_abc, out.d_uvw);

    const double tiny_linear = getTPMinLinearDisplacement();
    const double tiny_angular = getTPMinAngularDisplacement();
    if (out.d_xyz >= tiny_linear) {
        out.feed_axes = CANON_FEED_XYZ;
    } else if (out.d_uvw >= tiny_linear) {
        out.feed_axes = CANON_FEED_UVW;
    } else if (out.d_abc >= tiny_angular) {
        out.feed_axes = CANON_FEED_ABC;
    }
    return out;
}

/**
 * Finds velocity limits / expected minimum motion time for the given linear displacement.
 * @note all values are in EXTERNAL (TP) units, not canon units (see explanation of "path" units vs normal axis units).
 */
VelData getStraightVelocity(DisplacementData const &disp_data)
{
    // Start with safe default values
    VelData out{};

    PositionArray t{};
    for (size_t i=0; i < t.size(); ++i)
    {
        double maxvel = axis_max_velocity[i];
        bool have_vel = maxvel > 0 && axis_valid(i);
        t[i] = have_vel ? disp_data.displacement[i] / maxvel : 0.0;
    }

    out.t_max_path = fmax(max(t), 1e-7);
    out.v_max_path = disp_data.d_tot / out.t_max_path;
    out.vel_xyz = disp_data.d_xyz / out.t_max_path;
    out.vel_abc = disp_data.d_abc / out.t_max_path;
    out.vel_uvw = disp_data.d_uvw / out.t_max_path;

    return out;
}

/**
 * Get the limiting acceleration for a displacement from the current position to the given position.
 * returns a single acceleration that is the minimum of all axis accelerations.
 * @note all units are in EXTERNAL units (not canon units), see explanation of "path" vs axis units
 */
AccelData getStraightAcceleration(
    DisplacementData const &disp_data)
{
    // Start with safe default values
    AccelData out{};

    PositionArray t2{};
    for (size_t i=0; i < t2.size(); ++i)
    {
        double maxaccel = axis_max_acceleration[i];
        bool have_accel = maxaccel > 0 && axis_valid(i);
        t2[i] = have_accel ? disp_data.displacement[i] / maxaccel : 0.0;
    }

    out.t2max = fmax(max(t2), 1e-7);
    out.a_max_path = disp_data.d_tot / out.t2max;
    return out;
}

int axis_valid(int n) {
    return emc_traj_axis_mask & (1<<n);
}
