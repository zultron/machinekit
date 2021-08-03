#ifndef ARC_FEED_UTIL_HH
#define ARC_FEED_UTIL_HH

#include <math.h>
#include <posemath_fwd.h>
#include <stdio.h>

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

namespace interp_shared {
struct CircleAngle {
    double principal_angle;
    int full_turns;
    bool clockwise;
    bool has_implicit_full_turn; //!< Indicates an extra full turn added to compensate for the arc angle being below the fuzz threshold

    double full_angle() const {
        return principal_angle + 2.* M_PI * full_turns;
    }
    /** Returns the "command" form of turns, where -1 means 0 turns but in the clockwise direction */
    double get_turn_cmd() const {
        return clockwise ? full_turns - 1: full_turns;
    }

    static CircleAngle find_arc_angle_in_plane(double start1, double start2, double center1, double center2, int g2_g3_turns, double end1, double end2, bool allow_implicit_full_turn)
    {
        CircleAngle out{};
        if (g2_g3_turns == 0) {
            return out;
        }
        double theta1 = atan2(start2-center2, start1-center1);
        double theta2 = atan2(end2-center2, end1-center1);
        out.clockwise = g2_g3_turns < 0;

        // Check if the angles are large enough in the "wrong" direction that they're out of the "fuzz" zone
        if(out.clockwise) {
            out.full_turns = g2_g3_turns + 1; // Remove the extra turn leftover from the g2/g3 "turn" value
            // Detect a zero crossing and adjust the angle so that the difference produces the principal angle
            if (theta1 < 0 && theta2 > 0) {
                theta1 += 2.*M_PI;
            }
        } else {
            // Expect a positive angle for a counterclockwise move. If this angle
            // is more negative than the fuzz tolerance, add 1 rotation to be
            // within [0, 2pi]
            out.full_turns = g2_g3_turns - 1; // Remove the extra turn leftover from the g2/g3 "turn" value
            if (theta1 > 0 && theta2 < 0) {
                theta1 -= 2.*M_PI;
            }
        }
        out.principal_angle = theta2 - theta1;
        // If after all the angle adjustments we're left with a principal angle that's smaller than the fuzz amount, we add a full turn.
        // This is not a great heuristic (since the endpoints could be significantly far apart, and ), but it's at least consistent.
        if (allow_implicit_full_turn) {
            if (fabs(out.principal_angle) < CIRCLE_FUZZ) {
                out.full_turns += out.clockwise ? -1 : 1;
                out.has_implicit_full_turn = true;
            }
        }
        return out;
    }
};

} // namespace interp_shared
#endif // ARC_FEED_UTIL_HH
