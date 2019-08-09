#include "greatest.h"
#include "emcpos.h"
#include "posemath.h"

#define ASSERT_EMCPOSE_IN_RANGE(exp, got, tol) ASSERT(greatest_compare_EmcPose(exp, got, tol) >= 0)
#define ASSERT_PM_CARTESIAN_IN_RANGE(exp, got, tol) ASSERT(greatest_ompare_PmCartesian(exp, got, tol) >= 0)

int greatest_compare_EmcPose(EmcPose exp, EmcPose got, double tol)
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

int greatest_compare_PmCartesian(PmCartesian exp, PmCartesian got, double tol)
{
    ASSERT_IN_RANGE(exp.x, got.x, tol);
    ASSERT_IN_RANGE(exp.y, got.y, tol);
    ASSERT_IN_RANGE(exp.z, got.z, tol);
    return GREATEST_TEST_RES_PASS;
}
