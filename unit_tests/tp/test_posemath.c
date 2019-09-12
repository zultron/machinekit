#include "posemath.h"
#include "greatest.h"
#include "rtapi.h"
#include "math.h"
#include "motion_debug.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST test_pmCartCartCompare_self()
{
    PmCartesian start = {10.0, 2.0, -3.45};

    // Always self-compare positive
    ASSERT(pmCartCartCompare(&start, &start));
    PASS();
}

TEST test_pmCartCartCompare_small()
{
    PmCartesian start = {DOUBLE_FUZZ, DOUBLE_FUZZ, DOUBLE_FUZZ};

    PmCartesian large_diff=start;
    large_diff.z += 1.0;
    ASSERT(pmCartCartCompare(&start, &large_diff) == 0);

    // Always self-compare positive
    ASSERT(pmCartCartCompare(&start, &start));

    // NOTE: the initial point is <0,0,0> so that initial magnitude is small,
    // and adding the fuzz value does not round away the difference.
    PmCartesian too_small=start;
    too_small.y += nextafter(CART_FUZZ, 0.0);
    ASSERT(pmCartCartCompare(&start, &too_small) == 1);

    PmCartesian big_enough=start;
    big_enough.x += nextafter(CART_FUZZ, 1.0);
    ASSERT(pmCartCartCompare(&start, &big_enough) == 0);

    PASS();
}

TEST test_pmCartCartDot_parallel()
{
    PmCartesian zero = {0.0,0.0,0.0};
    double pmdot;
    pmCartCartDot(&zero, &zero, &pmdot);
    ASSERT_FLOAT_EQ(pmdot, 0.0);

    PmCartesian one = {1.0,-2.0,0.5};
    pmCartCartDot(&one, &one, &pmdot);
    ASSERT_FLOAT_EQ(pmdot, 5.25);
    PASS();
}

TEST test_pmCartCartDot_perpendicular()
{
    PmCartesian v1 = {cos(M_PI_4), sin(M_PI_4), 0.0};
    PmCartesian v2 = {sin(M_PI_4), -cos(M_PI_4), 0.0};
    double pmdot;
    pmCartCartDot(&v1, &v2, &pmdot);
    ASSERT_FLOAT_EQ(pmdot, 0.0);

    PASS();
}

TEST test_pmCartInfNorm()
{
    PmCartesian v1 ={3.25, -1, 2.0};
    PmCartesian v2 = v1;
    double v1_inf;
    pmCartInfNorm(&v1, &v1_inf);
    ASSERT_FLOAT_EQ(v1_inf, 3.25);

    v2.y = -4;
    double v2_inf;
    pmCartInfNorm(&v2, &v2_inf);
    ASSERT_FLOAT_EQ(v2_inf, 4);

    PASS();
}

static const PmCartesian v1 = {0.0011892247147802617, 0.019855106334368509, -0.00050476166977278807};
static const PmCartesian v2 = {-0.15453530685917072, -0.28437966952533078, -0.15744435569802101};
static const double k = -0.21253452789880767;
static const double mag_v1 = 0.019897092433684791;
static const double mag_v2 = 0.35991899456205328;
static const double mag_diff = 0.37608372040023119;
static const double dot = -0.0057506939081026628;
static const PmCartesian cross = {-0.0032696183809693856, 0.00026524021852782629, 0.0027301236187225355};
static const PmCartesian diff = {-0.15572453157395097, -0.30423477585969927, -0.15693959402824822};
static const PmCartesian sum = {-0.15334608214439047, -0.26452456319096229, -0.1579491173677938};
static const PmCartesian v1_neg = {-0.0011892247147802617, -0.019855106334368509, 0.00050476166977278807};
static const PmCartesian v1_mult_k = {-0.0002527513133214171, -0.0042198956511556368, 0.00010727928318657337};
static const PmCartesian v1_div_k = {-0.0055954424278133177, -0.093420615137988119, 0.002374963140168529};
static const PmCartesian elem_mult = {-0.00018377720622307753, -0.005646388577758019, 7.9471875878433867e-05};
static const PmCartesian elem_div = {-0.0076954887459084794, -0.069819007693164015, 0.0032059686581646868};

TEST test_PmCartesian_arithmetic()
{
    double pm_mag_v1=0.0;
    pmCartMag(&v1, &pm_mag_v1);
    ASSERT_IN_RANGE(1e-16, mag_v1, pm_mag_v1);

    double pm_mag_v2=0.0;
    pmCartMag(&v2, &pm_mag_v2);
    ASSERT_IN_RANGE(mag_v2, 1e-16, pm_mag_v2);

    double pm_mag_sq=0.0;
    pmCartMagSq(&v1, &pm_mag_sq);
    ASSERT_FLOAT_EQ(pow(mag_v1, 2), pm_mag_sq);

    double pm_disp;
    ASSERT_EQ(PM_OK, pmCartCartDisp(&v1, &v2, &pm_disp));
    ASSERT_FLOAT_EQ(mag_diff, pm_disp);

    PmCartesian pmsum;
    ASSERT_EQ(PM_OK, pmCartCartAdd(&v1, &v2, &pmsum));
    ASSERT_PMCARTESIAN_IN_RANGE(sum, pmsum, CART_FUZZ);

    PmCartesian pm_diff;
    ASSERT_EQ(PM_OK, pmCartCartSub(&v2, &v1, &pm_diff));
    ASSERT_PMCARTESIAN_IN_RANGE(diff, pm_diff, CART_FUZZ);

    PmCartesian pm_mult_k;
    ASSERT_EQ(PM_OK, pmCartScalMult(&v1, k, &pm_mult_k));
    ASSERT_PMCARTESIAN_IN_RANGE(v1_mult_k, pm_mult_k, CART_FUZZ);

    PmCartesian pm_div_k;
    ASSERT_EQ(PM_OK, pmCartScalDiv(&v1, k, &pm_div_k));
    ASSERT_PMCARTESIAN_IN_RANGE(v1_div_k, pm_div_k, CART_FUZZ);

    PmCartesian pm_neg;
    ASSERT_EQ(PM_OK, pmCartNeg(&v1, &pm_neg));
    ASSERT_PMCARTESIAN_IN_RANGE(v1_neg, pm_neg, CART_FUZZ);

    double pm_dot=0.0;
    pmCartCartDot(&v1, &v2, &pm_dot);
    ASSERT_FLOAT_EQ(pm_dot, dot);

    PmCartesian pm_cross;
    pmCartCartCross(&v1, &v2, &pm_cross);
    ASSERT_PMCARTESIAN_IN_RANGE(pm_cross, cross, CART_FUZZ);

    PmCartesian pm_elem_mult;
    pmCartCartMult(&v1, &v2, &pm_elem_mult);
    ASSERT_PMCARTESIAN_IN_RANGE(pm_elem_mult, elem_mult, CART_FUZZ);

    PmCartesian pm_elem_div;
    pmCartCartDiv(&v1, &v2, &pm_elem_div);
    ASSERT_PMCARTESIAN_IN_RANGE(pm_elem_div, elem_div, CART_FUZZ);

    PASS();
}

TEST test_pmCartUnit()
{
    PmCartesian pm_u1;
    ASSERT_FALSE(pmCartUnit(&v1, &pm_u1));

    double rx = pm_u1.x / v1.x;
    double ry = pm_u1.y / v1.y;
    double rz = pm_u1.z / v1.z;

    // Verify that unit vector is in the same direction
    ASSERT_FLOAT_EQ(rx, ry);
    ASSERT_FLOAT_EQ(ry, rz);
    ASSERT_FLOAT_EQ(rz, rx);

    double pm_umag;
    ASSERT_FALSE(pmCartMag(&pm_u1, &pm_umag));

    // And that the magnitude is close enough
    ASSERT_IN_RANGE(1.0, 1e-16, pm_umag);

    // Show that the self-modifying version is the same output
    PmCartesian ueq = v1;
    ASSERT_FALSE(pmCartUnitEq(&ueq));
    ASSERT_PMCARTESIAN_IN_RANGE(ueq, pm_u1, CART_FUZZ);
    PASS();
}

TEST test_pmCartAbs()
{
    PmCartesian const p_mixed = {-0.1, 2.0, -5.0};
    PmCartesian const exp_abs = {0.1, 2.0, 5.0};

    PmCartesian pm_abs;
    ASSERT_FALSE(pmCartAbs(&p_mixed, &pm_abs));
    ASSERT(pm_abs.x >= 0.0);
    ASSERT(pm_abs.y >= 0.0);
    ASSERT(pm_abs.z >= 0.0);
    ASSERT_PMCARTESIAN_IN_RANGE(exp_abs, pm_abs, CART_FUZZ);
    PASS();
}

TEST test_pmCartScalAddSub()
{
    PmCartesian const p_start = {-0.1, 2.0, -5.0};
    double const d = 2.5;
    PmCartesian const exp_add = {2.4, 4.5, -2.5};

    PmCartesian p_add, p_sub;
    ASSERT_FALSE(pmCartScalAdd(&p_start, d, &p_add));
    ASSERT_PMCARTESIAN_IN_RANGE(exp_add, p_add, CART_FUZZ);
    ASSERT_FALSE(pmCartScalSub(&p_add, d, &p_sub));
    ASSERT_PMCARTESIAN_IN_RANGE(p_start, p_sub, CART_FUZZ);
    PASS();
}

SUITE(pm_cart_basics)
{
    RUN_TEST(test_pmCartCartCompare_self);
    RUN_TEST(test_pmCartCartCompare_small);
    RUN_TEST(test_pmCartCartDot_parallel);
    RUN_TEST(test_pmCartCartDot_perpendicular);
    RUN_TEST(test_pmCartInfNorm);
    RUN_TEST(test_PmCartesian_arithmetic);
    RUN_TEST(test_pmCartUnit);
    RUN_TEST(test_pmCartAbs);
    RUN_TEST(test_pmCartScalAddSub);
}

TEST test_pmCircleInit_simple()
{
    PmCircle c;
    PmCartesian start = {10.0,0, 0};
    PmCartesian end = start;
    PmCartesian center = {0,0,0};
    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    ASSERT_FLOAT_EQ(c.angle, 2.0 * M_PI);
    ASSERT_FLOAT_EQ(c.radius, 10.0);

    PASS();
}

TEST test_pmCircleInit()
{
    PmCircle c;
    PmCartesian center = {2.2, 3.3, 4.4};

    const double nominal_radius = 0.5;
    const double theta1 = 0.123;
    const double theta2 = 1.456;

    PmCartesian start = {0.0, nominal_radius * cos(theta1), nominal_radius * sin(theta1)};
    pmCartCartAddEq(&start, &center);
    PmCartesian end = {0.0, nominal_radius * cos(theta2), nominal_radius * sin(theta2)};
    pmCartCartAddEq(&end, &center);

    PmCartesian normal = {1,0,0};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    ASSERT_IN_RANGE(c.angle, theta2-theta1, CART_FUZZ);
    ASSERT_IN_RANGE(c.radius, nominal_radius, CART_FUZZ);

    PASS();
}

TEST test_pmCircleInit_spiral_short()
{
    PmCircle c;
    PmCartesian center = {0.0, 0.0, 0.0};
    PmCartesian start = {1,0, 0};
    PmCartesian end = {0, 1 + 0.3,0};

    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    ASSERT_IN_RANGE(M_PI_2, c.angle, CART_FUZZ);
    ASSERT_IN_RANGE(0.3, c.spiral, CART_FUZZ);

    PASS();
}

TEST test_pmCircleInit_spiral_long()
{
    PmCircle c;
    PmCartesian center = {0.0, 0.0, 0.0};
    PmCartesian start = {1,0, 0};
    PmCartesian end = {-1-0.3, 0,0};

    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 3);

    ASSERT_IN_RANGE(M_PI * 7, c.angle, CART_FUZZ);
    ASSERT_IN_RANGE(0.3, c.spiral, CART_FUZZ);

    PASS();
}

TEST test_pmCircleInit_helix()
{
    PmCircle c;
    // Assume center is zero for ease of writing the test
    const PmCartesian center = {1.0, 2.0, 3.0};

    PmCartesian start = {1,0, 0};
    PmCartesian end = {1.0,0,0.4};
    pmCartCartAddEq(&start, &center);
    pmCartCartAddEq(&end, &center);

    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    PmCartesian const expect_rHelix = {0,0,0.4};

    ASSERT_IN_RANGE(2.0 * M_PI, c.angle, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(expect_rHelix, c.rHelix, CART_FUZZ);
    const double expect_radius = 1.0;

    // Radius should be constant for an ideal helix
    for (double angle = 0.0; angle <= M_PI * 2.0; angle += M_PI / 3.0) {
        PmCartesian p = {0};
        pmCirclePoint(&c, 0.0, &p);
        pmCartCartSubEq(&p, &center);
        // Ignore perpendicular component
        p.z=0;
        double r;
        pmCartMag(&p, &r);

        ASSERT_FLOAT_EQ(expect_radius, r);
    }

    PASS();
}


TEST test_pmCirclePoint_helix_spiral()
{
    PmCircle c;
    // Assume center is zero for ease of writing the test
    const PmCartesian center = {1.0, 2.0, 3.0};

    PmCartesian start = {1,0, 0};
    PmCartesian end = {0.0,-1.2,0.4};
    pmCartCartAddEq(&start, &center);
    pmCartCartAddEq(&end, &center);

    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    {
        PmCartesian p0 = {0};
        pmCirclePoint(&c, 0.0, &p0);
        PmCartesian const expect_p0 = {2.0, 2.0, 3.0};
        ASSERT_PMCARTESIAN_IN_RANGE(expect_p0, p0, CART_FUZZ);
        PmCartesian start_quick;
        pmCircleStartPoint(&c, &start_quick);
        ASSERT_PMCARTESIAN_IN_RANGE(expect_p0, start_quick, CART_FUZZ);
    }{
        PmCartesian p1 = {0};
        pmCirclePoint(&c, M_PI_2, &p1);
        PmCartesian const expect_p1 = {1.0, 2.0 + 1.0 + 0.2/3, 3 + 0.4/3};
        ASSERT_PMCARTESIAN_IN_RANGE(expect_p1, p1, CART_FUZZ);
    }{
        PmCartesian p2 = {0};
        pmCirclePoint(&c, M_PI, &p2);
        PmCartesian const expect_p2 = {1.0-1.0-0.2*2./3, 2.0, 3 + 0.4*2.0/3};
        ASSERT_PMCARTESIAN_IN_RANGE(expect_p2, p2, CART_FUZZ);
    }

    PASS();
}


TEST test_pmCircleStretch()
{
    PmCircle c;
    PmCartesian center = {0.0, 0.0, 0.0};

    const double nominal_radius = 0.5;
    const double theta1 = M_PI_4;
    const double theta2 = 3.0 / 8.0 * M_PI;

    PmCartesian start = {nominal_radius * cos(theta1), nominal_radius * sin(theta1), 0.0};
    PmCartesian end = {nominal_radius * cos(theta2), nominal_radius * sin(theta2), 0.0};
    PmCartesian normal = {0,0,1};

    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    PmCartesian sample_before;
    pmCirclePoint(&c, M_PI / 8.0, &sample_before);

    // Stretch to extend past the end point
    pmCircleStretch(&c, M_PI, 0);

    PmCartesian sample_end_stretch;
    pmCirclePoint(&c, M_PI / 8.0, &sample_end_stretch);

    ASSERT_PMCARTESIAN_IN_RANGE(sample_before, sample_end_stretch, CART_FUZZ);

    // Stretch to extend past the starting point
    pmCircleStretch(&c, M_PI * 5.0/4.0, 1);

    PmCartesian sample_start_stretch;
    pmCirclePoint(&c, M_PI / 8.0 + M_PI_4, &sample_start_stretch);

    ASSERT_PMCARTESIAN_IN_RANGE(sample_before, sample_start_stretch, CART_FUZZ);
    PASS();
}

TEST test_pmCircleStretchTangent_begin()
{
    PmCircle c;
    PmCartesian center = {0.0, 0.0, 0.0};

    const double start_radius = 1.0;
    const double final_radius = 1.0;
    const double theta1 = 0;
    const double theta2 = M_PI_2;

    PmCartesian start = {start_radius * cos(theta1), start_radius * sin(theta1), 0.0};
    PmCartesian end = {final_radius * cos(theta2), final_radius * sin(theta2), 1.0};
    PmCartesian normal = {0,0,1};

    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    PmCartesian u_before;
    pmCircleTangentVector(&c, M_PI * 3.0 / 8.0, &u_before);

    ASSERT_FALSE(pmCircleStretch(&c, M_PI * 3.0 / 8.0, 0));

    PmCartesian u_after;
    ASSERT_FALSE(pmCircleTangentVector(&c, M_PI * 3.0 / 8.0 , &u_after));

    ASSERT_PMCARTESIAN_IN_RANGE(u_before, u_after, CART_FUZZ);
    PASS();
}

TEST test_pmCircleStretchTangent_end()
{
    PmCircle c;
    PmCartesian center = {0.0, 0.0, 0.0};

    const double start_radius = 1.0;
    const double final_radius = 0.1;
    const double theta1 = 0;
    const double theta2 = M_PI_2;

    PmCartesian start = {start_radius * cos(theta1), start_radius * sin(theta1), 0.0};
    PmCartesian end = {final_radius * cos(theta2), final_radius * sin(theta2), 1.0};
    PmCartesian normal = {0,0,1};

    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    PmCartesian u_before;
    pmCircleTangentVector(&c, M_PI / 8.0, &u_before);
    PmCartesian p_before;
    pmCirclePoint(&c, M_PI / 8.0, &p_before);

    // Stretch to extend past the starting point
    ASSERT_FALSE(pmCircleStretch(&c, M_PI * 3.0 / 8.0, 1));

    PmCartesian u_after;
    ASSERT_FALSE(pmCircleTangentVector(&c, 0, &u_after));
    PmCartesian p_after;
    ASSERT_FALSE(pmCirclePoint(&c, 0, &p_after));

    ASSERT_PMCARTESIAN_IN_RANGE(u_before, u_after, CART_FUZZ);
    PASS();
}

SUITE(pm_circle) {
    RUN_TEST(test_pmCircleInit_simple);
    RUN_TEST(test_pmCircleInit);
    RUN_TEST(test_pmCircleInit_spiral_short);
    RUN_TEST(test_pmCircleInit_spiral_long);
    RUN_TEST(test_pmCircleInit_helix);
    RUN_TEST(test_pmCirclePoint_helix_spiral);
    RUN_TEST(test_pmCircleStretch);
}

TEST test_pmCartLineInit_simple()
{
    PmCartLine tst;
    PmCartesian start = {0,0,0};
    PmCartesian end = {1,2,3};

    pmCartLineInit(&tst, &start, &end);

    ASSERT_FLOAT_EQ(tst.tmag, sqrt(1 + 4 + 9));
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);
    PASS();
}

TEST test_pmCartLineInit_ex1()
{
    PmCartLine tst;
    const PmCartesian start = {
        0.51320891452372031,
        0.3513817033641376,
        0.30203143034775765,
    };
    const PmCartesian end = {
        0.96211392505131188,
        0.20784727108932183,
        0.73266325488481165,
    };
    const double mag = 0.63840552161564434;
    pmCartLineInit(&tst, &start, &end);

    ASSERT_FLOAT_EQ(tst.tmag, mag);
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);

    const PmCartesian expect_uvec = {
        0.70316592718610194,
        -0.22483269241087719,
        0.6745427631127513
    };
    ASSERT_PMCARTESIAN_EQ(expect_uvec, tst.uVec);
    PASS();
}

TEST test_pmCartLineInit_ex2()
{
    PmCartLine tst;
    const PmCartesian start = {
        1.2196110251384671,
        -0.99958732370407333,
        0.41726932038630371,
    };
    const PmCartesian end = {
        0.29786615882106027,
        0.91903453415863678,
        0.7079824705329727,
    };
    const double mag = 2.1483103983640843;
    const PmCartesian expect_uvec = {
        -0.42905572072792919,
        0.89308409963649593,
        0.1353217627992882
    };

    pmCartLineInit(&tst, &start, &end);

    ASSERT_FLOAT_EQ(tst.tmag, mag);
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);
    ASSERT_PMCARTESIAN_EQ(expect_uvec, tst.uVec);
    PASS();
}

TEST test_pmCartLineInit_small_belowfuzz()
{
    PmCartLine tst;
    const PmCartesian start = {
        0,0,0
    };
    const double almost_fuzz = nextafter(CART_FUZZ, 0.0);
    const PmCartesian end = {
        almost_fuzz,
        -almost_fuzz,
        almost_fuzz,
    };
    pmCartLineInit(&tst, &start, &end);
    double mag=0.0;
    pmCartMag(&end, &mag);

    // Store the actual magnitude
    ASSERT_FLOAT_EQ(tst.tmag, mag);
    ASSERT(tst.tmag_zero > 0);
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);
    PASS();
}

TEST test_pmCartLineInit_small_abovefuzz()
{
    PmCartLine tst;
    const PmCartesian start = {
        0,0,0
    };
    const double above_fuzz = nextafter(CART_FUZZ, 1.0);
    const PmCartesian end = {
        above_fuzz,
        -above_fuzz,
        above_fuzz,
    };
    pmCartLineInit(&tst, &start, &end);

    ASSERT(tst.tmag > 0.0);
    ASSERT(tst.tmag_zero == 0);
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);
    PASS();
}

TEST test_pmCartLinePoint()
{
    PmCartLine tst;
    const PmCartesian start = {
        0.020215567195396768,
        -0.01189543243460452,
        0.025741132979321263,
    };
    const PmCartesian end = {
        4.6752714631010335,
        -2.6867637044820922,
        -3.6444762156871819,
    };
    const double s = 1.4828560169111997;
    const PmCartesian expect_point = {1.0816167547670967, -0.62179323474589543, -0.81110661700091891};

    pmCartLineInit(&tst, &start, &end);
    PmCartesian point = {0,0,0};
    pmCartLinePoint(&tst, s, &point);

    ASSERT(tst.tmag > 0.0);
    ASSERT(tst.tmag_zero == 0);
    ASSERT_PMCARTESIAN_IN_RANGE(start, tst.start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(end, tst.end, CART_FUZZ);
    ASSERT_PMCARTESIAN_EQ(expect_point, point);
    PASS();
}

static enum greatest_test_res check_pmCartCartOrthonormal_valid(PmCartesian u, PmCartesian v)
{
    double m1;
    pmCartMagSq(&u, &m1);
    ASSERT_IN_RANGE(1.0, m1, CART_FUZZ);
    double m2;
    pmCartMagSq(&v, &m2);
    ASSERT_IN_RANGE(1.0, m2, CART_FUZZ);
    double d;
    pmCartCartDot(&u, &v, &d);
    ASSERT_IN_RANGE(0, d, CART_FUZZ);
    PASS();
}

TEST test_pmCartCartOrthonormal()
{
    PmCartesian const a = {1,0,0};
    PmCartesian const b = {1,1,0};

    PmCartesian a_hat, b_hat;

    pmCartUnit(&a, &a_hat);
    pmCartUnit(&b, &b_hat);

    PmCartesian u, v;
    pmCartCartOrthonormal(&a_hat, &b_hat, &u, &v);

    PmCartesian u_exp = {1,0,0};
    ASSERT_PMCARTESIAN_IN_RANGE(u_exp, u, CART_FUZZ);
    PmCartesian v_exp = {0,1,0};
    ASSERT_PMCARTESIAN_IN_RANGE(v_exp, v, CART_FUZZ);
    CHECK_CALL(check_pmCartCartOrthonormal_valid(u, v));

    PmCartesian a_close = a;
    a_close.x+=CART_FUZZ;
    PmCartesian a_close_hat;
    pmCartUnit(&a_close, &a_close_hat);

    // Verify graceful handling of colinear vectors (only need 1 basis vector, 2nd vector is negligible length)
    pmCartCartOrthonormal(&a_hat, &a_hat, &u, &v);
    double mag = -1;
    pmCartMag(&v, &mag);
    ASSERT_IN_RANGE(0, mag, CART_FUZZ);

    double mag2 = -1;
    pmCartCartOrthonormal(&a_close_hat, &a_hat, &u, &v);
    pmCartMag(&v, &mag2);
    ASSERT_IN_RANGE(0, mag2, CART_FUZZ);

    PASS();
}

SUITE(pm_line) {
    RUN_TEST(test_pmCartLineInit_simple);
    RUN_TEST(test_pmCartLineInit_ex1);
    RUN_TEST(test_pmCartLineInit_ex2);
    RUN_TEST(test_pmCartLineInit_small_belowfuzz);
    RUN_TEST(test_pmCartLineInit_small_abovefuzz);
    RUN_TEST(test_pmCartLinePoint);
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(pm_cart_basics);
    RUN_SUITE(pm_circle);
    RUN_SUITE(pm_line);
    RUN_TEST(test_pmCartCartOrthonormal);
    RUN_TEST(test_pmCircleStretchTangent_begin);
    RUN_TEST(test_pmCircleStretchTangent_end);
    GREATEST_MAIN_END();
}
