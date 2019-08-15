#include "tp_debug.h"
#include "greatest.h"
#include "blendmath.h"
#include "tp_types.h"
#include "math.h"
#include "rtapi.h"
#include "joint_util.h"
#include "motion_debug.h"
#include "tc.h"
#include "tcq.h"
#include "mock_standalone_tp.h"
#include "state_tag.h"
#include "mot_priv.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"
EmcPose newXYZ(double x, double y, double z, EmcPose last)
{
    EmcPose out = last;
    PmCartesian t = {x,y,z};
    out.tran = t;
    return out;
}

struct state_tag_t line_tag(int line_no)
{
    struct state_tag_t out;
    out.fields[GM_FIELD_LINE_NUMBER] = line_no;
    return out;
}

TEST test_mock_tp_init() {
    CREATE_MOCK_TP();
    ASSERT(tp);

    ASSERT_EQ(0, tcqLen(&tp->queue));

    deleteMockTP(&tp);
    PASS();
}

TEST test_create_single_line() {
    CREATE_MOCK_TP();

    EmcPose pt1 = {
        {
            3.0,
            5.0,
            8.0
        },
        0,0,0,0,0,0
    };

    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.0);

    tpAddLine(tp, pt1, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 10.0, 0, 0, 0, 0, line_tag(1));
    ASSERT_EQ(1, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt1, tp->goalPos, TP_POS_EPSILON);

    deleteMockTP(&tp);
    PASS();
}

TEST test_create_two_exact_lines() {
    CREATE_MOCK_TP();

    EmcPose pt1 = {
        {
            3.0,
            5.0,
            8.0
        },
        0,0,0,0,0,0
    };

    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.0);

    tpAddLine(tp, pt1, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 10.0, 0, 0, 0, 0, line_tag(1));
    ASSERT_EQ(1, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt1, tp->goalPos, TP_POS_EPSILON);

    EmcPose pt2 = newXYZ(1,4,9,pt1);
    tpAddLine(tp, pt2, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 10.0, 0, 0, 0, 0, line_tag(2));
    ASSERT_EQ(2, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);

    deleteMockTP(&tp);
    PASS();
}

TEST test_create_two_blended_lines() {
    CREATE_MOCK_TP();

    EmcPose pt0 = {0};

    EmcPose pt1 = newXYZ(1,0,0,pt0);
    tpSetTermCond(tp, TC_TERM_COND_PARABOLIC, 0.0);

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddLine(tp, pt1, EMC_MOTION_TYPE_FEED, 10.0, 10.0, 20.0, 0, 0, 0, 0, line_tag(1));
    ASSERT_EQ(1, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt1, tp->goalPos, TP_POS_EPSILON);

    EmcPose pt2 = newXYZ(1,1,0,pt1);
    tpAddLine(tp, pt2, EMC_MOTION_TYPE_FEED, 10.0, 10.0, 20.0, 0, 0, 0, 0, line_tag(2));
    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(3, tcqLen(&tp->queue));

    // check blend properties here
    TC_STRUCT const *blend_tc = tcqItem(&tp->queue, 1);
    ASSERT_EQ(TC_SPHERICAL, blend_tc->motion_type);
    ASSERT_IN_RANGE(0.5, blend_tc->coords.arc.xyz.radius, TP_POS_EPSILON);

    deleteMockTP(&tp);
    PASS();
}

/**
 * Prove that two circular segments in XY plane are
 */
TEST test_create_tangent_arcs() {
    CREATE_MOCK_TP();
    tpSetTermCond(tp, TC_TERM_COND_PARABOLIC, 0.0);

    EmcPose pt0 = {0};
    EmcPose pt1 = newXYZ(1,1,0,pt0);
    EmcPose pt2 = newXYZ(2,0,0,pt1);
    PmCartesian center1 = {1,0,0};
    PmCartesian center2 = {1,0,0};
    PmCartesian normal = {0,0,-1};

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 19.0, 0, 0, 0, line_tag(1));
    tpAddCircle(tp, pt2, center2, normal, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 19.0, 0, 0, 0, line_tag(2));

    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(2, tcqLen(&tp->queue));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);
    ASSERT_EQ(TC_TERM_COND_TANGENT, prev_tc->term_cond);

    PmCartesian const exp_tan_vec = {1,0,0};
    PmCartesian got_prev_tan;
    tcGetEndTangentUnitVector(prev_tc, &got_prev_tan);
    ASSERT_PMCARTESIAN_IN_RANGE(exp_tan_vec, got_prev_tan, CART_FUZZ);
    PmCartesian got_this_tan;
    tcGetStartTangentUnitVector(this_tc, &got_this_tan);
    ASSERT_PMCARTESIAN_IN_RANGE(exp_tan_vec, got_this_tan, CART_FUZZ);

    deleteMockTP(&tp);
    PASS();
}

TEST test_slightly_convex_arcs() {
    CREATE_MOCK_TP();
    tpSetTermCond(tp, TC_TERM_COND_PARABOLIC, 0.0);

    EmcPose const pt0 = {0};
    EmcPose const pt1 = newXYZ(1,1,0,pt0);
    EmcPose const pt2 = newXYZ(2,0,0,pt1);
    double const dc = 0.001;
    // Displace slightly along 45 deg lines
    PmCartesian center1 = {1 - dc, dc,0};
    PmCartesian center2 = {1 + dc, dc,0};
    PmCartesian const normal = {0,0,-1};

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 19.0, 0, 0, 0, line_tag(1));
    tpAddCircle(tp, pt2, center2, normal, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 19.0, 0, 0, 0, line_tag(2));

    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(3, tcqLen(&tp->queue));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);
    ASSERT_EQ(TC_TERM_COND_TANGENT, prev_tc->term_cond);

    deleteMockTP(&tp);
    PASS();
}

TEST test_find_blend_points_and_tangents()
{
    // Create arc segments
    CREATE_MOCK_TP();
    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.0);

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {0};
    EmcPose const pt1 = newXYZ(2,0,0,pt0);
    EmcPose const pt2 = newXYZ(2,2,0,pt1);

    PmCartesian center1 = {1, 0, 0};
    PmCartesian center2 = {2, 1, 0};
    PmCartesian const normal1 = {0,1,0};
    PmCartesian const normal2 = {0,0,1};

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal1, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(1));
    tpAddCircle(tp, pt2, center2, normal2, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(2));

    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(2, tcqLen(&tp->queue));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);

    blend_boundary_t bb = {0};
    double const s_b = M_PI / 6;

    find_blend_points_and_tangents(s_b, prev_tc, this_tc, &bb);

    // Verify that the positions and tangents make sense
    PmCartesian const exp_P1 = {1. + cos(s_b), 0, sin(s_b)};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_P1, bb.P1, CART_FUZZ);
    PmCartesian const exp_P2 = {2. + sin(s_b), 1-cos(s_b), 0};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_P2, bb.P2, CART_FUZZ);

    PmCartesian const exp_u1 = {sin(s_b), 0, -cos(s_b)};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_u1, bb.u1, CART_FUZZ);
    PmCartesian const exp_u2 = {cos(s_b), sin(s_b), 0};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_u2, bb.u2, CART_FUZZ);

    deleteMockTP(&tp);
    PASS();
}

TEST test_find_blend_intermediates()
{
    // Create arc segments
    CREATE_MOCK_TP();
    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.0);

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {0};
    EmcPose const pt1 = newXYZ(2,0,0,pt0);
    EmcPose const pt2 = newXYZ(4,0,0,pt1);

    PmCartesian center1 = {1, 0, 0};
    PmCartesian center2 = {3, 0, 0};
    PmCartesian const normal1 = {0,0,1};
    PmCartesian const normal2 = {0,0,1};

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal1, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(1));
    tpAddCircle(tp, pt2, center2, normal2, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(2));

    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(2, tcqLen(&tp->queue));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);

    blend_boundary_t bb = {0};
    double const s_b = M_PI / 4;

    find_blend_points_and_tangents(s_b, prev_tc, this_tc, &bb);

    blend_intermediate_points_t bp_out;
    find_blend_intermediate_segments(&bb, &bp_out);

    PmCartesian exp_u_mid = {1,0,0};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_u_mid, bp_out.u_mid, CART_FUZZ);
    ASSERT_IN_RANGE(0.17157287525380996, bp_out.d, TP_POS_EPSILON);

    deleteMockTP(&tp);
    PASS();
}

TEST test_optimize_biarc_blend_size()
{
    // Create arc segments
    CREATE_MOCK_TP();
    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.005);

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {0};
    EmcPose const pt1 = newXYZ(2,0,0,pt0);
    EmcPose const pt2 = newXYZ(4,0,0,pt1);

    PmCartesian center1 = {1, 0, 0};
    PmCartesian center2 = {3, 0, 0};
    PmCartesian const normal1 = {0,0,1};
    PmCartesian const normal2 = {0,0,1};

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal1, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(1));
    tpAddCircle(tp, pt2, center2, normal2, 0, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 15.0, 0, 0, 0, line_tag(2));

    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);
    ASSERT_EQ(2, tcqLen(&tp->queue));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);

    PmCartesian vel_bound = getXYZVelBounds();
    PmCartesian acc_bound = getXYZAccelBounds();

    blend_boundary_t bb = {0};
    blend_intermediate_points_t bp = {0};
    BlendGeom3 geom = {0};
    BlendParameters param = {0};
    optimize_biarc_blend_size(
        prev_tc,
        this_tc,
        &bb,
        &bp,
        1.0,
        &vel_bound,
        &acc_bound,
        &geom,
        &param);

    deleteMockTP(&tp);
    PASS();
}



int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_TEST(test_mock_tp_init);
    RUN_TEST(test_create_single_line);
    RUN_TEST(test_create_two_exact_lines);
    RUN_TEST(test_create_two_blended_lines);
    RUN_TEST(test_create_tangent_arcs);
    RUN_TEST(test_slightly_convex_arcs);
    RUN_TEST(test_find_blend_points_and_tangents);
    RUN_TEST(test_find_blend_intermediates);
    RUN_TEST(test_optimize_biarc_blend_size);
    GREATEST_MAIN_END();        /* display results */
}
