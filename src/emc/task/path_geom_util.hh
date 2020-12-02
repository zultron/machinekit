#ifndef PATH_GEOM_UTIL_HH
#define PATH_GEOM_UTIL_HH

#include "canon_enums.hh"
#include "positionarray.hh"
using namespace interp_shared;

struct DisplacementData {
    DisplacementData() = default;
    PositionArray displacement;
    /** Total straight-line distance traveled along the line (6D). */
    double d_tot;

    double d_xyz;
    double d_abc;
    double d_uvw;

    CanonFeedAxisReferenceType feed_axes;
};

/**
 *  Holds results of velocity calculations for a straight segment.
 *  @todo This should really be refactored into a more general structure, but
 *  this means tearing up the getStraightXXX functions, which probably means
 *  converting to canon_position operators.
 *  @todo replace redundant data with member functions (to simplify GetStraightXXX functions)
 */
struct VelData {
    VelData() = default;

    /** Total time required to complete move for all axes */
    double t_max_path;

    /** 9D augmented velocity */
    double v_max_path;

    double vel_xyz;
    double vel_abc;
    double vel_uvw;
};

struct AccelData{
    AccelData() = default;

    double t2max;
    double a_max_path;
};

double hypot3(double a, double b, double c);

DisplacementData getStraightDisplacement(const PositionArray & start_pt, PositionArray const &goal_pt);
DisplacementData getArcDisplacement(const PositionArray & start_pt, PositionArray const &goal_pt, double total_xyz_length);
VelData getStraightVelocity(DisplacementData const &disp_data);
AccelData getStraightAcceleration(DisplacementData const &disp_data);
void addCircularPathDisplacement(DisplacementData &disp_data, double total_xyz_length);

int axis_valid(int n);
#endif // PATH_GEOM_UTIL_HH
