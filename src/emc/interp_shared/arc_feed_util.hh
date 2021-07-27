#ifndef ARC_FEED_UTIL_HH
#define ARC_FEED_UTIL_HH

#include <math.h>
#include <posemath_fwd.h>
#include <stdio.h>

namespace interp_shared {
struct CircleAngle {
    double principal_angle;
    int full_turns;
    bool clockwise;

    double full_angle() const {
        return principal_angle + 2.* M_PI * full_turns;
    }
    /** Returns the "command" form of turns, where -1 means 0 turns but in the clockwise direction */
    double get_turn_cmd() const {
        return clockwise ? full_turns - 1: full_turns;
    }
};

inline CircleAngle find_arc_angle_in_plane(double start1, double start2, double center1, double center2, double end1, double end2, int g2_g3_turns)
{
    CircleAngle out{};
    double theta1 = atan2(start2-center2, start1-center1);
    double theta2 = atan2(end2-center2, end1-center1);
    out.principal_angle = theta2 - theta1;
    out.clockwise = g2_g3_turns < 0;

    // Check if the angles are large enough in the "wrong" direction that they're out of the "fuzz" zone
    if(out.clockwise) {
        out.full_turns = g2_g3_turns + 1; // Remove the extra turn leftover from the g2/g3 "turn" value
        // Expect a negative angle for a clockwise move. so if this angle is
        // more positive than the fuzz tolerance, subtract 1 rotation to get a
        // negative angle within [-2pi,0]
        if (out.principal_angle >= CIRCLE_FUZZ) {
            out.principal_angle -= (2.*M_PI);
        }
    } else {
        // Expect a positive angle for a counterclockwise move. If this angle
        // is more negative than the fuzz tolerance, add 1 rotation to be
        // within [0, 2pi]
        out.full_turns = g2_g3_turns - 1; // Remove the extra turn leftover from the g2/g3 "turn" value
        if (out.principal_angle <= -CIRCLE_FUZZ) {
            out.principal_angle += (2.*M_PI);
        }
    }
    // If after all the angle adjustments we're left with a principal angle that's smaller than the fuzz amount, we add a full turn.
    // This is not a great heuristic (since the endpoints could be significantly far apart, and ), but it's at least consistent.
    if (fabs(out.principal_angle) < CIRCLE_FUZZ) {
        out.full_turns += out.clockwise ? -1 : 1;
    }
    return out;
}
}
#endif // ARC_FEED_UTIL_HH
