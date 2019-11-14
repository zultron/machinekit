#include "tp_debug.h"
#include "greatest.h"
#include "mock_standalone_tp.h"
#include "motion_struct.h"
#include "tp_priv.h"
#include "greatest_compare.h"
#include "posemath.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST test_estimateParabolicBlendPerformance_perpendicular()
{
    CREATE_MOCK_TP_ACC(8.0);
    ASSERT(tp);
    tpSetTermCond(tp, TC_TERM_COND_STOP, 0.05);

    EmcPose pt1 = {
        {
            5.0,
            0.0,
            0.0
        },
        0,0,0,0,0,0
    };

    tpAddLine(tp, pt1, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 10.0, 0, 0, 0, 0, line_tag(1));
    ASSERT_EQ(1, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt1, tp->goalPos, TP_POS_EPSILON);

    EmcPose pt2 = newXYZ(5,5,0,pt1);
    tpAddLine(tp, pt2, EMC_MOTION_TYPE_FEED, 1.0, 1.0, 10.0, 0, 0, 0, 0, line_tag(2));
    ASSERT_EQ(2, tcqLen(&tp->queue));
    ASSERT_EMCPOSE_IN_RANGE(pt2, tp->goalPos, TP_POS_EPSILON);

    // KLUDGE force parabolic blending
    TC_STRUCT * prev_tc = tcqItem(&tp->queue, 0);
    TC_STRUCT * this_tc = tcqItem(&tp->queue, 1);

    prev_tc->term_cond = TC_TERM_COND_PARABOLIC;
    this_tc->blend_prev = true;

    double v_est = estimateParabolicBlendPerformance(prev_tc, this_tc);
    ASSERT_IN_RANGE(pmSqrt(2.0)/2.0, v_est, CART_FUZZ);

    deleteMockTP(&tp);
    PASS();
}


int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_TEST(test_estimateParabolicBlendPerformance_perpendicular);
    GREATEST_MAIN_END();        /* display results */
}
