#ifndef GREATEST_COMPARE_H
#define GREATEST_COMPARE_H

#include "greatest.h"
#include "emcpos.h"
#include "posemath.h"

// KLUDGE string is generated every time even if there are no errors
#define ASSERT_EMCPOSE_IN_RANGE(EXP, GOT, TOL) \
do { \
    EmcPose exp = EXP; \
    EmcPose got = GOT; \
    double tol = TOL; \
    char msg[4096]={0}; \
    sprintf(msg, "expected %s: {{%g, %g, %g}, %g, %g, %g, %g, %g, %g}, got %s: {{%g, %g, %g}, %g, %g, %g, %g, %g, %g}", #EXP, exp.tran.x, exp.tran.y, exp.tran.z, exp.a, exp.b, exp.c, exp.u, exp.v, exp.w,\
        #GOT, got.tran.x, got.tran.y, got.tran.z, got.a, got.b, got.c, got.u, got.v, got.w); \
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

#define ASSERT_PMVECTOR_IN_RANGE(EXP, GOT, TOL) \
do { \
    PmVector exp = EXP; \
    PmVector got = GOT; \
    double tol = TOL; \
    char msg[4096]={0}; \
    sprintf(msg, "expected %s: {{%g, %g, %g}, %g, %g, %g, %g, %g, %g}, got %s: {{%g, %g, %g}, %g, %g, %g, %g, %g, %g}", #EXP, exp.ax[0], exp.ax[1], exp.ax[2], exp.ax[3], exp.ax[4], exp.ax[5], exp.ax[6], exp.ax[7], exp.ax[8],\
        #GOT, got.ax[0], got.ax[1], got.ax[2], got.ax[3], got.ax[4], got.ax[5], got.ax[6], got.ax[7], got.ax[8]); \
    ASSERT_IN_RANGE(exp.ax[0], got.ax[0], tol); \
    ASSERT_IN_RANGE(exp.ax[1], got.ax[1], tol); \
    ASSERT_IN_RANGE(exp.ax[2], got.ax[2], tol); \
    ASSERT_IN_RANGE(exp.ax[3], got.ax[3], tol); \
    ASSERT_IN_RANGE(exp.ax[4], got.ax[4], tol); \
    ASSERT_IN_RANGE(exp.ax[5], got.ax[5], tol); \
    ASSERT_IN_RANGE(exp.ax[6], got.ax[6], tol); \
    ASSERT_IN_RANGE(exp.ax[7], got.ax[7], tol); \
    ASSERT_IN_RANGE(exp.ax[8], got.ax[8], tol); \
} while (0)

// KLUDGE string is generated every time even if there are no errors
#define ASSERT_PMCARTESIAN_IN_RANGEm(MSG, EXP, GOT, TOL) \
do { \
    PmCartesian exp = EXP; \
    PmCartesian got = GOT; \
    double tol = TOL; \
    char msg[4096]={0}; \
    sprintf(msg, "%s%sexpected %s: {%g, %g, %g}, got %s: {%g, %g, %g}", MSG, MSG ? ": " : "", #EXP, exp.x, exp.y, exp.z, #GOT, got.x, got.y, got.z); \
    ASSERT_IN_RANGEm(msg, exp.x, got.x, tol); \
    ASSERT_IN_RANGEm(msg, exp.y, got.y, tol); \
    ASSERT_IN_RANGEm(msg, exp.z, got.z, tol); \
} while (0)

#define ASSERT_PMCARTESIAN_EQ(EXP, GOT) ASSERT_PMCARTESIAN_IN_RANGE(EXP, GOT, CART_FUZZ)
#define ASSERT_PMCARTESIAN_IN_RANGE(EXP, GOT, TOL) ASSERT_PMCARTESIAN_IN_RANGEm("", EXP, GOT, TOL)

#endif // GREATEST_COMPARE_H
