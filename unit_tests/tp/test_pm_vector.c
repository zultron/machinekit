#include "posemath.h"
#include "pm_vector.h"
#include "greatest.h"
#include "rtapi.h"
#include "math.h"
#include "motion_debug.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST test_VecVecDot_parallel()
{
    PmVector zero = {{0.0,0.0,0.0}};
    double dot_zero=  VecVecDot(&zero, &zero);
    ASSERT_FLOAT_EQ(0.0, dot_zero);

    PmVector some_vec = {{1.0,-2.0,0.5}};
    const double dot_some_vec = VecVecDot(&some_vec, &some_vec);
    ASSERT_FLOAT_EQ(5.25, dot_some_vec);
    PASS();
}

TEST test_VecVecDot_perpendicular()
{
    PmVector v1 = {{cos(M_PI_4), sin(M_PI_4), 0.0}};
    PmVector v2 = {{sin(M_PI_4), -cos(M_PI_4), 0.0}};
    double v_dot = VecVecDot(&v1, &v2);
    ASSERT_FLOAT_EQ(0.0, v_dot);

    PASS();
}

static const PmVector v1 = {{0.0011892247147802617, 0.019855106334368509, -0.00050476166977278807, .1, .2, .3, .4, .5, .6}};
static const PmVector v2 = {{-0.15453530685917072, -0.28437966952533078, -0.15744435569802101, .2, -.4, .6, -.8, 1.0, -1.2}};
static const double k = -0.21253452789880767;
static const double exp_mag_v1 = 0.9541466838423297014;
static const double exp_mag_v2 = 0.35991899456205328;
static const double exp_mag_diff = 2.351901138387852974;
static const double exp_dot = -0.4257506939081028285;
static const PmVector exp_diff = {{-0.15572453157395097, -0.30423477585969927, -0.15693959402824822, .1, -.6, .3, -1.2, .5, -1.8}};
static const PmVector exp_sum = {{-0.15334608214439047, -0.26452456319096229, -0.1579491173677938, 0.3, -0.2, 0.9, -0.4, 1.5, -0.6}};
static const PmVector exp_v1_mult_k = {{
    -0.0002527513133214170492,
    -0.004219895651155635904,
    0.0001072792831865733399,
    -0.02125345278988076403,
    -0.04250690557976152806,
    -0.06376035836964227821,
    -0.08501381115952305612,
    -0.1062672639494038063,
    -0.1275207167392845564,
    }};

TEST test_PmVector_arithmetic()
{
    double pm_mag_v1 = VecMag(&v1);
    ASSERT_IN_RANGE(exp_mag_v1, pm_mag_v1, 1e-16);

    double pm_mag_v2 = VecMag(&v2);
    ASSERT_IN_RANGE(exp_mag_v2, 1e-16, pm_mag_v2);

    double pm_mag_sq = VecMagSq(&v1);
    ASSERT_IN_RANGE(pow(exp_mag_v1, 2), pm_mag_sq, 1e-15);

    double pm_disp = VecVecDisp(&v1, &v2);
    ASSERT_IN_RANGE(exp_mag_diff, pm_disp, 1e-15);

    PmVector pmsum;
    ASSERT_EQ(PM_OK, VecVecAdd(&v1, &v2, &pmsum));
    ASSERT_PMVECTOR_IN_RANGE(exp_sum, pmsum, CART_FUZZ);

    PmVector pm_diff;
    ASSERT_EQ(PM_OK, VecVecSub(&v2, &v1, &pm_diff));
    ASSERT_PMVECTOR_IN_RANGE(exp_diff, pm_diff, CART_FUZZ);

    PmVector pm_mult_k;
    ASSERT_EQ(PM_OK, VecScalMult(&v1, k, &pm_mult_k));
    ASSERT_PMVECTOR_IN_RANGE(exp_v1_mult_k, pm_mult_k, CART_FUZZ);

    double pm_dot = VecVecDot(&v1, &v2);
    ASSERT_IN_RANGE(exp_dot, pm_dot, 1e-15);

    PASS();
}

TEST test_VecUnit()
{
    PmVector pm_u1;
    ASSERT_FALSE(VecUnit(&v1, &pm_u1));

    double rx = pm_u1.ax[0] / v1.ax[0];
    double ry = pm_u1.ax[1] / v1.ax[1];
    double rz = pm_u1.ax[2] / v1.ax[2];

    // Verify that unit vector is in the same direction
    ASSERT_FLOAT_EQ(rx, ry);
    ASSERT_FLOAT_EQ(ry, rz);
    ASSERT_FLOAT_EQ(rz, rx);

    double pm_umag = VecMag(&pm_u1);

    // And that the magnitude is close enough
    ASSERT_IN_RANGE(1.0, 1e-16, pm_umag);

    // Show that the self-modifying version is the same output
    PmVector ueq = v1;
    ASSERT_FALSE(VecUnitEq(&ueq));
    ASSERT_PMVECTOR_IN_RANGE(ueq, pm_u1, CART_FUZZ);
    PASS();
}

TEST test_VecAbs()
{
    PmVector const p_mixed = {{-0.1, 2.0, -5.0}};
    PmVector const exp_abs = {{0.1, 2.0, 5.0}};

    PmVector pm_abs;
    ASSERT_FALSE(VecAbs(&p_mixed, &pm_abs));
    ASSERT(pm_abs.ax[0] >= 0.0);
    ASSERT(pm_abs.ax[1] >= 0.0);
    ASSERT(pm_abs.ax[2] >= 0.0);
    ASSERT_PMVECTOR_IN_RANGE(exp_abs, pm_abs, CART_FUZZ);
    PASS();
}

static enum greatest_test_res check_VecVecOrthonormal_valid(PmVector u, PmVector v)
{
    double m1 = VecMagSq(&u);
    ASSERT_IN_RANGE(1.0, m1, CART_FUZZ);
    double m2 = VecMagSq(&v);
    ASSERT_IN_RANGE(1.0, m2, CART_FUZZ);
    double d = VecVecDot(&u, &v);
    ASSERT_IN_RANGE(0, d, CART_FUZZ);
    PASS();
}

TEST test_VecVecOrthonormal()
{
    PmVector const a = {{1,0,0}};
    PmVector const b = {{1,1,0}};

    PmVector a_hat, b_hat;

    VecUnit(&a, &a_hat);
    VecUnit(&b, &b_hat);

    PmVector u, v;
    VecVecOrthonormal(&a_hat, &b_hat, &u, &v);

    PmVector u_exp = {{1,0,0}};
    ASSERT_PMVECTOR_IN_RANGE(u_exp, u, CART_FUZZ);
    PmVector v_exp = {{0,1,0}};
    ASSERT_PMVECTOR_IN_RANGE(v_exp, v, CART_FUZZ);
    CHECK_CALL(check_VecVecOrthonormal_valid(u, v));

    PmVector a_close = a;
    a_close.ax[0]+=CART_FUZZ;
    PmVector a_close_hat;
    VecUnit(&a_close, &a_close_hat);

    // Verify graceful handling of colinear vectors (only need 1 basis vector, 2nd vector is negligible length)
    VecVecOrthonormal(&a_hat, &a_hat, &u, &v);
    double mag = VecMag(&v);
    ASSERT_IN_RANGE(0, mag, CART_FUZZ);

    VecVecOrthonormal(&a_close_hat, &a_hat, &u, &v);
    double mag2 =VecMag(&v);
    ASSERT_IN_RANGE(0, mag2, CART_FUZZ);

    PASS();
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(test_VecVecDot_parallel);
    RUN_TEST(test_VecVecDot_perpendicular);
    RUN_TEST(test_PmVector_arithmetic);
    RUN_TEST(test_VecUnit);
    RUN_TEST(test_VecAbs);
    RUN_TEST(test_VecVecOrthonormal);
    GREATEST_MAIN_END();
}
