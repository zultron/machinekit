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

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

// Helpers for creating segments / tags / etc
#define CREATE_MOCK_TP() TP_STRUCT *tp = newMockTP(1000, 0.001)

int compare_pose(EmcPose exp, EmcPose got, double tol)
{
    ASSERT_IN_RANGE(exp.tran.x, got.tran.x, tol);
    ASSERT_IN_RANGE(exp.tran.y, got.tran.y, tol);
    ASSERT_IN_RANGE(exp.tran.z, got.tran.z, tol);
    ASSERT_IN_RANGE(exp.a, got.a, tol);
    ASSERT_IN_RANGE(exp.b, got.b, tol);
    ASSERT_IN_RANGE(exp.c, got.c, tol);
    ASSERT_IN_RANGE(exp.u, got.u, tol);
    ASSERT_IN_RANGE(exp.v, got.v, tol);
    ASSERT_IN_RANGE(exp.w, got.w, tol);
    return GREATEST_TEST_RES_PASS;
}

#define ASSERT_EMCPOSE_IN_RANGE(exp, got, tol) ASSERT(compare_pose(exp, got, tol) >= 0)

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

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_TEST(test_mock_tp_init);
    RUN_TEST(test_create_single_line);
    RUN_TEST(test_create_two_exact_lines);
    RUN_TEST(test_create_two_blended_lines);
    GREATEST_MAIN_END();        /* display results */
}
