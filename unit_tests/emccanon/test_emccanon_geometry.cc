#include "catch.hpp"
#include "canon.hh"
#include <sstream>
#include <cstdio>
#include <emc_statics.hh>
#include <emc_nml.hh>
#include <emccanon.hh>
#include <interp_testing_util.hh>
#include <interpl.hh>

TEST_CASE("EMCCANON hello world")
{
    setup_9axis_config();
    INIT_CANON();
    while(interp_list.get()) {}
    SET_FEED_RATE(100.0);
    auto *tag_msg = interp_list.get();
    REQUIRE(tag_msg);
    CHECK(tag_msg->type == EMC_TASK_SET_TAG_TYPE);
    ARC_FEED(0,1,2,3,4,5,6,7,8,9,1,2,3);
    auto *arc_msg = interp_list.get();
    REQUIRE(arc_msg);
    CHECK(arc_msg->type == EMC_TRAJ_CIRCULAR_MOVE_TYPE);
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

SCENARIO("A circular IJK arc with the same start / endpoint")
{
    DECL_INIT_TEST_INTERP();
    INIT_CANON();
    REQUIRE_INTERP_OK(test_interp, execute("F200"));
    while(interp_list.get()) {} // Discard any NML commands up to this point

    GIVEN("A clockwise, circular arc that ends at the start point") {
        REQUIRE_INTERP_OK(test_interp, execute("G2 X0 Y0 Z0 I1.0"));
        THEN("Expect a single circular move NML command as a result") {
            NMLmsg *msg = nullptr;
            // Discard any tag messages
            while ((msg = interp_list.get()) && msg->type == EMC_TASK_SET_TAG_TYPE) {}
            auto *arc_msg = (EMC_TRAJ_CIRCULAR_MOVE*) msg;
            REQUIRE(arc_msg);
            REQUIRE(arc_msg->type == EMC_TRAJ_CIRCULAR_MOVE_TYPE);
            // Only check these once we're sure the type is right
            const PM_CARTESIAN exp_center = {1.0,0.0,0.0};
            const PM_CARTESIAN exp_normal = {0.0,0.0,1.0};
            CHECK_THAT(arc_msg->center, Catch::Matchers::PoseMath::CartApproxMatcher(exp_center));
            CHECK_THAT(arc_msg->normal, Catch::Matchers::PoseMath::CartApproxMatcher(exp_normal));
        }
    }
}

SCENARIO("Max feed / acceleration of circular arcs")
{
    DECL_INIT_TEST_INTERP();
    INIT_CANON();
    REQUIRE_INTERP_OK(test_interp, execute("F60"));
    while(interp_list.get()) {} // Discard any NML commands up to this point

    GIVEN("A planar semi-circular arc with no spiral or helical component") {
        const double eff_radius=0.25;
        REQUIRE_INTERP_OK(test_interp, execute("G2 X0.5 Y0 Z0 I0.25"));
        THEN("Expect a single circular move NML command as a result") {
            NMLmsg *msg = nullptr;
            while ((msg = interp_list.get()) && msg->type == EMC_TASK_SET_TAG_TYPE) {}
            auto *arc_msg = (EMC_TRAJ_CIRCULAR_MOVE*) msg;
            REQUIRE(arc_msg);
            REQUIRE(arc_msg->type == EMC_TRAJ_CIRCULAR_MOVE_TYPE);
            // Only check these once we're sure the type is right
            const PM_CARTESIAN exp_center = {.25,0.0,0.0};
            const PM_CARTESIAN exp_normal = {0.0,0.0,1.0};
            CHECK_THAT(arc_msg->center, Catch::Matchers::PoseMath::CartApproxMatcher(exp_center));
            CHECK_THAT(arc_msg->normal, Catch::Matchers::PoseMath::CartApproxMatcher(exp_normal));

            // Allow ~86% of the maximum theoretical acceleration budget for normal acceleration
            const double exp_acc_normal_max = 15. * sqrt(3.)/2.; //IPS^2
            const double exp_v_max_planar = sqrt(exp_acc_normal_max * eff_radius);
            CHECK(arc_msg->vel == Approx(1.));
            CHECK(arc_msg->ini_maxvel == Approx(exp_v_max_planar));
        }
    }
    // TODO other cases
}
