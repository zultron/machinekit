#ifndef GREATEST_COMPARE_H
#define GREATEST_COMPARE_H

#include "greatest.h"
#include "emcpos.h"
#include "posemath.h"

#define ASSERT_EMCPOSE_IN_RANGE(EXP, GOT, TOL) \
do { \
    EmcPose exp = EXP; \
    EmcPose got = GOT; \
    double tol = TOL; \
    ASSERT_IN_RANGE(exp.tran.x, got.tran.x, tol); \
    ASSERT_IN_RANGE(exp.tran.y, got.tran.y, tol); \
    ASSERT_IN_RANGE(exp.tran.z, got.tran.z, tol); \
    ASSERT_IN_RANGE(exp.a, got.a, tol); \
    ASSERT_IN_RANGE(exp.b, got.b, tol); \
    ASSERT_IN_RANGE(exp.c, got.c, tol); \
    ASSERT_IN_RANGE(exp.u, got.u, tol); \
    ASSERT_IN_RANGE(exp.v, got.v, tol); \
    ASSERT_IN_RANGE(exp.w, got.w, tol); \
} while (0)

#define ASSERT_PMCARTESIAN_IN_RANGE(EXP, GOT, TOL) \
do { \
    PmCartesian exp = EXP; \
    PmCartesian got = GOT; \
    double tol = TOL; \
    ASSERT_IN_RANGE(exp.x, got.x, tol); \
    ASSERT_IN_RANGE(exp.y, got.y, tol); \
    ASSERT_IN_RANGE(exp.z, got.z, tol); \
} while (0)

#endif // GREATEST_COMPARE_H
