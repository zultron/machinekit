#include "catch.hpp"
#include "canon.hh"
#include <sstream>
#include <cstdio>
#include <emc_statics.hh>
#include <emc_nml.hh>
#include <emccanon.hh>
#include <interp_testing_util.hh>

TEST_CASE("EMCCANON hello world")
{
    emcStatus->motion.traj.axis_mask = 0xF;
    INIT_CANON();
    SET_FEED_RATE(100.0);
    ARC_FEED(0,1,2,3,4,5,6,7,8,9,1,2,3);
}

/**
 * Check if the voronoi regions of a line segment are used correctionly for 9D lines
 *
 *       .                        .
 *       .         (2)            .
 *       .                        .
 * (1)   x-------line-segment-----x      (4)
 *       . P0                     . P1
 *       .                        .
 *       .          (3)           .
 *       .                        .
 *
 *  If the test point lies in regions 2 / 3 in the above diagram, we find the
 *  perpendicular distance to the line.
 *  In region (1), the distance to the segment is R - P0
 *  In region (4), the distance to the segment is R - P1
 */
SCENARIO("Test for various distance measurements in distance check")
{
    emcStatus->motion.traj.axis_mask = 0xF;
    INIT_CANON();
    SET_FEED_RATE(100.0);
    THEN("XYZ-only points should have minimum distance from either the line or one of the end points depending on the voronoi region") {
        PositionArray const P0 = {1,1,0,0,0,0,0,0,0};
        PositionArray const P_region1_below = {1,0.99,0,0,0,0,0,0,0};
        PositionArray const P_region1_above = {0.99,1.0,0,0,0,0,0,0,0};
        PositionArray const P_region2 = {1,1.5,0,0,0,0,0,0,0};
        PositionArray const P_region3 = {1.5,1,0,0,0,0,0,0,0};
        PositionArray const P_region4_below = {2.01,2,0,0,0,0,0,0,0};
        PositionArray const P_region4_above = {2,2.01,0,0,0,0,0,0,0};
        PositionArray const P1 = {2,2,0,0,0,0,0,0,0};

        CHECK_FUZZ(distance_to_segment(P0, P1, P_region1_above), 0.01);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region1_below), 0.01);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region2), sqrt(2)/4);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region3), sqrt(2)/4);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region4_above), 0.01);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region4_below), 0.01);
    }

    // KLUDGE hard-coded in emccanon too
    const double exp_mmPerDeg = get_naivecam_mm_per_deg();

    THEN("rotary axis motion should be converted to linear equivalent distance using the canon NCD radius") {
        PositionArray const P0 = {1,1,0,2,0,0,0,0,0};
        PositionArray const P_region1_below = {1,0.99,0,1,0,0,0,0,0};
        PositionArray const P_region1_above = {0.99,1.0,0,1,0,0,0,0,0};
        PositionArray const P_region2 = {1,1.5,0,2.25,0,0,0,0,0};
        PositionArray const P_region3 = {1.5,1,0,2.25,0,0,0,0,0};
        PositionArray const P_region3_2 = {1.5,1.5,0,2.0,0,0,0,0,0};
        PositionArray const P_region4_below = {2.01,2,0,4,0,0,0,0,0};
        PositionArray const P_region4_above = {2,2.01,0,4,0,0,0,0,0};
        PositionArray const P1 = {2,2,0,3,0,0,0,0,0};

        CHECK_FUZZ(distance_to_segment(P0, P1, P_region1_above), hypot(0.01, exp_mmPerDeg));
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region1_below), hypot(0.01, exp_mmPerDeg));
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region2), sqrt(2)/4);
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region3), sqrt(2)/4);
        PositionArray const exp_dist_3_2 = {1./6,1./6,0,-1./3*exp_mmPerDeg,0,0,0,0,0};
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region3_2), mag(exp_dist_3_2));
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region4_above), hypot(0.01, exp_mmPerDeg));
        CHECK_FUZZ(distance_to_segment(P0, P1, P_region4_below), hypot(0.01, exp_mmPerDeg));
    }
}
