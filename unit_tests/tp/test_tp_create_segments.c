/**
 * @file test_tp_create_segments.c
 * 
 * Unit tests for higher-level functions in TP around creating / modifying motion segments.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

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

    blend_control_points_t bp_out;
    find_blend_intermediate_segments(&bb, &bp_out);

    PmCartesian exp_u_mid = {1,0,0};
    ASSERT_PMCARTESIAN_IN_RANGE(exp_u_mid, bp_out.u_mid, CART_FUZZ);
    ASSERT_IN_RANGE(0.17157287525380996, bp_out.d, TP_POS_EPSILON);

    deleteMockTP(&tp);
    PASS();
}

static greatest_test_res setup_initial_line(TP_STRUCT *tp, double v_req, double a_max, double blend_tolerance)
{
    tpSetTermCond(tp, TC_TERM_COND_STOP, blend_tolerance);
    double const v_max = 2.0;

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {{0,-2,0},0,0,0,0,0,0};
    EmcPose const pt1 = newXYZ(0,0,0,pt0);

    tpSetPos(tp, &pt0);
    tpSetId(tp, 1);
    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddLine(tp, pt1, EMC_MOTION_TYPE_FEED, v_req, v_max, a_max, 0, 0, 0, 0, line_tag(1));

    TC_STRUCT const * tc = tcqItem(&tp->queue, 0);
    PmCartesian const u_exp = {0,1,0};
    PmCartesian u_act;
    tcGetEndTangentUnitVector(tc, &u_act);
    ASSERT_PMCARTESIAN_IN_RANGE(u_exp, u_act, TP_POS_EPSILON);

    ASSERT_EQ(1, tcqLen(&tp->queue));
    return GREATEST_TEST_RES_PASS;
}

static greatest_test_res setup_initial_circle(TP_STRUCT *tp, double v_req, double a_max, double blend_tolerance)
{
    tpSetTermCond(tp, TC_TERM_COND_STOP, blend_tolerance);
    // KLUDGE guess at safe tangential acceleration value
    double const a_t_max = a_max * BLEND_ACC_RATIO_TANGENTIAL;
    double const a_n_max = a_max * BLEND_ACC_RATIO_NORMAL;
    double const v_max = 2.0;

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {{-2,0,0},0,0,0,0,0,0};
    EmcPose const pt1 = newXYZ(0,0,0,pt0);

    // Circle tangent should be
    PmCartesian center1 = {-1, 0, 0};
    PmCartesian const normal1 = {0,0,1};

    tpSetPos(tp, &pt0);
    tpSetId(tp, 1);
    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal1, 0, EMC_MOTION_TYPE_FEED, v_req, v_max, a_t_max, a_n_max, 0, 0, line_tag(1));

    TC_STRUCT const * tc = tcqItem(&tp->queue, 0);
    ASSERT(tc);
    ASSERT_FLOAT_EQ(v_max, tc->maxvel);

    PmCartesian const u_exp = {0,1,0};
    PmCartesian u_act;
    tcGetEndTangentUnitVector(tc, &u_act);
    ASSERT_PMCARTESIAN_IN_RANGE(u_exp, u_act, TP_POS_EPSILON);

    ASSERT_EQ(1, tcqLen(&tp->queue));
    return GREATEST_TEST_RES_PASS;
}


static greatest_test_res add_circle_from_center_normal(TP_STRUCT *tp, double v_req, double a_max, double blend_tolerance, PmCartesian center2, PmCartesian normal2, int turns)
{
    tpSetTermCond(tp, TC_TERM_COND_STOP, blend_tolerance);
    // KLUDGE guess at safe tangential acceleration value
    double const a_t_max = a_max * BLEND_ACC_RATIO_TANGENTIAL;
    double const a_n_max = a_max * BLEND_ACC_RATIO_NORMAL;
    double const v_max = 2.0;

    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt1 = {0};
    EmcPose pt2 = pt1;
    pmCartScalMult(&center2, 2.0, &pt2.tran);

    // Allow caller to specify non-unit normals
    pmCartUnitEq(&normal2);
    ASSERT_EMCPOSE_IN_RANGE(pt1, tp->goalPos, TP_POS_EPSILON);

    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt2, center2, normal2, turns, EMC_MOTION_TYPE_FEED, v_req, v_max, a_t_max, a_n_max, 0, 0, line_tag(2));

    TC_STRUCT const * tc = tcqItem(&tp->queue, 0);
    ASSERT(tc);
    ASSERT_FLOAT_EQ(v_max, tc->maxvel);

    ASSERT_EQ(2, tcqLen(&tp->queue));
    return GREATEST_TEST_RES_PASS;
}


/**
 * Populate a TP with two circles, the first one's endpoint is at 0,0,0.
 * Choose a normal2 that works for center2.
 */
static greatest_test_res setup_two_xy_unit_circles(TP_STRUCT *tp, double v_req, double blend_tolerance, PmCartesian center2, PmCartesian normal2, int test_id)
{
    tpSetTermCond(tp, TC_TERM_COND_STOP, blend_tolerance);
    // KLUDGE guess at safe tangential acceleration value
    double const a_t_max = 20.0 * BLEND_ACC_RATIO_TANGENTIAL;
    double const a_n_max = 20.0 * BLEND_ACC_RATIO_NORMAL;
    double const v_max = 2.0;


    // Unit radius to make angle -> arc length conversion easy
    EmcPose const pt0 = {{-2,0,0},0,0,0,0,0,0};
    EmcPose const pt1 = newXYZ(0,0,0,pt0);
    EmcPose pt2 = pt1;

    PmCartesian center1 = {-1, 0, 0};
    // Simple 180 deg arc
    pmCartScalMult(&center2, 2, &pt2.tran);
    PmCartesian const normal1 = {0,0,1};

    tpSetPos(tp, &pt0);
    // Ask for impossibly high velocity to force the arc to maximum size
    tpAddCircle(tp, pt1, center1, normal1, 0, EMC_MOTION_TYPE_FEED, v_req, v_max, a_t_max, a_n_max, 0, 0, line_tag(2*test_id+10));
    tpAddCircle(tp, pt2, center2, normal2, 0, EMC_MOTION_TYPE_FEED, v_req, v_max, a_t_max, a_n_max, 0, 0, line_tag(2*test_id + 11));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    ASSERT(prev_tc);
    ASSERT_FLOAT_EQ(v_max, prev_tc->maxvel);

    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(this_tc);
    ASSERT_FLOAT_EQ(v_max, this_tc->maxvel);

    ASSERT_EQ(2, tcqLen(&tp->queue));
    return GREATEST_TEST_RES_PASS;
}


static const biarc_solver_config_t cfg = {
    20,
    0.02,
    0.001,
    0.000001,
    1.0, // Ignore feed override scale for now
    0.001,
    0.0000001,
};

TEST test_optimize_biarc_blend_size()
{
    static const double BLEND_TOLERANCE = 0.005;
    CREATE_MOCK_TP();
    ASSERT(tp);

    ASSERT_EQ(0, tcqLen(&tp->queue));
    PmCartesian const center2 = {0,1,0};
    PmCartesian const normal2 = {0,0,1};

    CHECK_CALL(setup_two_xy_unit_circles(tp, 1.0, BLEND_TOLERANCE, center2, normal2, 0));

    TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
    ASSERT(prev_tc);
    ASSERT(this_tc);

    PmCartesian vel_bound = getXYZVelBounds();
    PmCartesian acc_bound = getXYZAccelBounds();

    BlendParameters param = {0};

    biarc_solver_results_t biarc_results = {0};

    tp_err_t res_opt = optimize_biarc_blend_size(
        prev_tc,
        this_tc,
        &cfg,
        &vel_bound,
        &acc_bound,
        &biarc_results,
        &param,
        tp->cycleTime);

    ASSERT_EQ(TP_ERR_OK, res_opt);
    ASSERT_EQ(BIARC_REACHED_TOLERANCE, biarc_results.result);
    // Verify that the midpoint is within tolerance
    double T;
    EmcPose P;
    tcGetStartpoint(this_tc, &P);
    pmCartCartDisp(&biarc_results.control_pts.P_mid, &P.tran, &T);
    ASSERT(T < BLEND_TOLERANCE + cfg.deviation_abs_tolerance);
    ASSERT(biarc_results.iterations < 10);

    deleteMockTP(&tp);
    PASS();
}


typedef struct {
    PmCartesian center;
    PmCartesian normal;
    long turns;
} CenterNormal;

TEST test_biarc_optimizer_performance()
{
    CREATE_MOCK_TP_ACC(8.0);
    ASSERT(tp);
    static const double BLEND_TOLERANCE = 0.05;
    static const double v_req = 1.0;

    static const CenterNormal cases[]={
        {{0.100000, 0.000293, 0.100000}, {0.000000, 0.000000, 1.000000}},
        {{0.084656,-0.053229, 0.100000}, {0.000000, 0.000000, 1.000000}},
        {{0.505793,-0.862655, 0.000000}, {0.000000, 0.000000, 1.000000}},
        {{-1,0.001,0},{0,0,1}},
        {{1,0,0},{0,0,-1}},
        {{1,0,0},{0,0,1}},
        {{.5,0,0},{0,0,1}},
        {{.1,0,0},{0,0,1}},
        {{.01,0,0},{0,0,1}},
        {{1,-0.1,0},{0,0,1}},
        {{1,-0.01,0},{0,0,1}},
        {{1,-0.001,0},{0,0,1}},
        {{1,-0.0001,0},{0,0,1}},
        {{1, 0.00000,0},{0,0,1}},
        {{1, 0.0001,0},{0,0,1}},
        {{1, 0.001,0},{0,0,1}},
        {{1, 0.01,0},{0,0,1}},
        {{1, 0.1,0},{0,0,1}},
        {{1,1,0},{0,0,1}},
        {{1,0,0},{0,1,0}},
        {{1,0,0},{0,-1,0}},
        {{0,0,1},{-1,0,0}},
        {{0,0,1},{1,0,0}},
        {{0,0,1},{0,1,0}},
        {{0,0,1},{0,-1,0}},
        {{0,1,1},{0,0,-1}},
        {{0,1,1},{0,0,1}},
        {{1,0,1},{0,1,0}},
        {{1,0,1},{0,-1,0}},
        {{0,.1,.1},{0,0,-1}},
        {{0,.1,.1},{0,0,1}},
        {{.1,0,.1},{0,1,0}},
        {{.1,0,.1},{0,-1,0}},
        {{1,0,0.5},{0,0,1}},
        {{1,1,1},{0,0,1}},
        };

    printf("\nCircle biarc blend tests\n");
    printf("%9s,%9s,%9s,%9s,%9s,%9s,%9s,%6s,%9s,%9s,%9s,%9s,%9s,%9s,%14s,\n",
            "type",
            "C_x",
            "C_y",
            "C_z",
            "N_x",
            "N_y",
            "N_z",
            "tolerance",
            "R_b",
            "R_plan",
            "v_plan",
            "blend_d",
            "theta",
            "iters",
            "status");
    for (int j=0; j<2; ++j) {
        for (unsigned i=0; i < (sizeof(cases)/sizeof (CenterNormal)); ++i) {
            resetMockTP(tp, 0.001);
            switch (j) {
            case 0:
                CHECK_CALL(setup_initial_line(tp, v_req, 8.0, BLEND_TOLERANCE));
                break;
            case 1:
                CHECK_CALL(setup_initial_circle(tp, v_req, 8.0, BLEND_TOLERANCE));
                break;
            }

            ASSERT_EQ(1, tcqLen(&tp->queue));
            CHECK_CALL(add_circle_from_center_normal(tp, 1.0, 8.0, BLEND_TOLERANCE, cases[i].center, cases[i].normal, cases[i].turns));

            TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
            TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
            ASSERT(prev_tc);
            ASSERT(this_tc);

            PmCartesian vel_bound = getXYZVelBounds();
            PmCartesian acc_bound = getXYZAccelBounds();

            BlendParameters param = {0};
            biarc_solver_results_t biarc_results = {0};

#ifdef DO_BIARC_SCAN
            printf(">Center %f %f %f, Normal %f %f %f\n",
                   cases[i].center.x,
                   cases[i].center.y,
                   cases[i].center.z,
                   cases[i].normal.x,
                   cases[i].normal.y,
                   cases[i].normal.z);
            scan_blend_properties(
                prev_tc,
                this_tc,
                &cfg,
                &vel_bound,
                &acc_bound,
                &biarc_results,
                &param,
                tp->cycleTime,
                0.0001);
#endif

            optimize_biarc_blend_size(
                prev_tc,
                this_tc,
                &cfg,
                &vel_bound,
                &acc_bound,
                &biarc_results,
                &param,
                tp->cycleTime);

            // Verify that the midpoint is within tolerance
            double T;
            EmcPose P;
            tcGetStartpoint(this_tc, &P);
            pmCartCartDisp(&biarc_results.control_pts.P_mid, &P.tran, &T);
            double v_plan = pmSqrt(param.a_n_max * biarc_results.R_plan);

            PmCartesian u1, u2;
            tcGetEndTangentUnitVector(prev_tc, &u1);
            tcGetStartTangentUnitVector(this_tc, &u2);
            param.theta = findIntersectionAngle(&u1, &u2);

            printf("%9s,%9f,%9f,%9f,%9f,%9f,%9f,%9g,%9f,%9f,%9f,%9f,%9f,%6d,%14s,\n",
                   j ? " arc-arc" : "line-arc",
                   cases[i].center.x,
                   cases[i].center.y,
                   cases[i].center.z,
                   cases[i].normal.x,
                   cases[i].normal.y,
                   cases[i].normal.z,
                   T,
                   biarc_results.Rb,
                   biarc_results.R_plan,
                   v_plan,
                   biarc_results.control_pts.d,
                   param.theta * 180./M_PI,
                   biarc_results.iterations,
                   biarc_result_to_str(biarc_results.result));
            ASSERT(biarc_results.R_plan > 0.0);
            if (biarc_results.result != BIARC_MIN_LENGTH_LIMITED) {
                ASSERT(T <= (BLEND_TOLERANCE + cfg.deviation_abs_tolerance));
                double real_tol = 0;
                tcFindBlendTolerance(prev_tc, this_tc, &real_tol);
                ASSERT(fabs(v_plan - v_req) / v_req < cfg.velocity_rel_tolerance  || fabs(T - real_tol) < cfg.deviation_abs_tolerance  || biarc_results.result == BIARC_CONVERGED);
            }
            ASSERT(biarc_results.iterations < 100);
        }
    }

    deleteMockTP(&tp);
    PASS();
}

static greatest_test_res solve_blend_series(
    TP_STRUCT *tp,
    const char *out_name,
    unsigned I_MAX,
    double BLEND_TOLERANCE,
    double v_req,
    double a_max,
    double R,
    double h,
    int use_init_arc)
{
    FILE *f = fopen(out_name,"w");
    ASSERT(f);
    fprintf(f, "%12s,%9s,%9s,%9s,%9s,%9s,%9s,%9s,%6s,%9s,%9s,%9s,%9s,%9s,%6s,%6s,\n",
            "seg_angle",
            "radius",
            "C_x",
            "C_y",
            "C_z",
            "N_x",
            "N_y",
            "N_z",
            "tolerance",
            "R_b",
            "R_plan",
            "v_plan",
            "blend_d",
            "theta",
            "iters",
            "status");
    for (unsigned i=0; i < I_MAX; ++i) {
        resetMockTP(tp, 0.001);
        switch (use_init_arc) {
        case 0:
            CHECK_CALL(setup_initial_line(tp, v_req, a_max, BLEND_TOLERANCE));
            break;
        case 1:
            CHECK_CALL(setup_initial_circle(tp, v_req, a_max, BLEND_TOLERANCE));
            break;
        }
        ASSERT_EQ(1, tcqLen(&tp->queue));

        double theta = (double)i/I_MAX * 2.0*M_PI;
        PmCartesian center2 = {R * cos(theta), R * sin(theta), h};
        PmCartesian normal2 = {0,0,1};
        CHECK_CALL(add_circle_from_center_normal(tp, v_req, a_max, BLEND_TOLERANCE, center2, normal2, 1));

        TC_STRUCT const * prev_tc = tcqItem(&tp->queue, 0);
        TC_STRUCT const * this_tc = tcqItem(&tp->queue, 1);
        ASSERT(prev_tc);
        ASSERT(this_tc);

        PmCartesian vel_bound = getXYZVelBounds();
        PmCartesian acc_bound = getXYZAccelBounds();

        BlendParameters param = {0};
        biarc_solver_results_t biarc_results = {0};

        optimize_biarc_blend_size(
            prev_tc,
            this_tc,
            &cfg,
            &vel_bound,
            &acc_bound,
            &biarc_results,
            &param,
            tp->cycleTime);

        // Verify that the midpoint is within tolerance
        double T;
        EmcPose P;
        tcGetStartpoint(this_tc, &P);
        pmCartCartDisp(&biarc_results.control_pts.P_mid, &P.tran, &T);
        double v_plan = pmSqrt(param.a_n_max * biarc_results.R_plan);

        PmCartesian u1, u2;
        tcGetEndTangentUnitVector(prev_tc, &u1);
        tcGetStartTangentUnitVector(this_tc, &u2);
        param.theta = findIntersectionAngle(&u1, &u2);

        fprintf(f, "%12f,%9f,%9f,%9f,%9f,%9f,%9f,%9f,%9g,%9f,%9f,%9f,%9f,%9f,%6d,%6d,\n",
                theta * 180./M_PI,
                R,
                center2.x,
                center2.y,
                center2.z,
                normal2.x,
                normal2.y,
                normal2.z,
                T,
                biarc_results.Rb,
                biarc_results.R_plan,
                v_plan,
                biarc_results.control_pts.d,
                param.theta * 180./M_PI,
                biarc_results.iterations,
                biarc_results.result);
    }
    fclose(f);
    return GREATEST_TEST_RES_PASS;
}

TEST test_biarc_optimizer_exercise()
{
    CREATE_MOCK_TP_ACC(8.0);
    ASSERT(tp);

    CHECK_CALL(solve_blend_series(tp, "/home/rellenberg/octave/biarc_blends/aa-short-small-helix.csv", 30000, 0.05, 1.0, 8.0, .1, .1, true));
    CHECK_CALL(solve_blend_series(tp, "/home/rellenberg/octave/biarc_blends/aa-tall-helix.csv", 60000, 0.05, 1.0, 8.0, 0.5, 2.0, true));
    CHECK_CALL(solve_blend_series(tp, "/home/rellenberg/octave/biarc_blends/aa-semicircle.csv", 30000, 0.05, 1.0, 8.0, 1, 0, true));

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
    RUN_TEST(test_biarc_optimizer_performance);
    RUN_TEST(test_biarc_optimizer_exercise);
    RUN_TEST(test_optimize_biarc_blend_size);
    GREATEST_MAIN_END();        /* display results */
}
