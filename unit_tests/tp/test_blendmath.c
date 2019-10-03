#include "tp_debug.h"
#include "greatest.h"
#include "blendmath.h"
#include "tp_types.h"
#include "math.h"
#include "rtapi.h"
#include "joint_util.h"
#include "motion_debug.h"
#include "tc.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();


#include "mock_rtapi.inc"

TEST test_findAccelScale()
{
    PmVector bounds = {{5.0, 0.0, 20.0, 0}};
    PmVector acc = {{200.0, 100.0, -1.0}};

    PmVector scale_out;
    findAccelScale(&acc, &bounds, &scale_out);

    ASSERT_FLOAT_EQ(40, scale_out.ax[0]);
    ASSERT_FLOAT_EQ(0, scale_out.ax[1]);
    ASSERT_FLOAT_EQ(1/20.0, scale_out.ax[2]);
    PASS();
}

TEST test_findMaxValueOnPlane_basic()
{
    PmVector const bounds_easy = {{10.0, 10.0, 10.0}};
    PmVector const l1 = {{1,0,0}};
    PmVector const l2 = {{-1,-1,0}};

    PmVector u1, u2, u, v, uv_harmonic_sq;
    VecUnit(&l1, &u1);
    VecUnit(&l2, &u2);
    VecVecOrthonormal(&u1, &u2, &u, &v);
    VecVecHarmonicAddSq(&u, &v, &uv_harmonic_sq);

    double b_max;
    findMaxValueOnPlane(&uv_harmonic_sq, &bounds_easy, &b_max);

    ASSERT_FLOAT_EQ(10.0, b_max);
    PASS();
}

TEST test_findMaxValueOnPlane_missing_y()
{
    PmVector const bounds_easy = {{10.0, 0.0, 20.0}};
    PmVector const l1 = {{1,0,3}};
    PmVector const l2 = {{-1,0,-5}};

    PmVector u1, u2, u, v, uv_harmonic_sq;
    VecUnit(&l1, &u1);
    VecUnit(&l2, &u2);
    VecVecOrthonormal(&u1, &u2, &u, &v);
    VecVecHarmonicAddSq(&u, &v, &uv_harmonic_sq);

    double b_max;
    findMaxValueOnPlane(&uv_harmonic_sq, &bounds_easy, &b_max);

    ASSERT_IN_RANGE(10.0, b_max, TP_POS_EPSILON);
    PASS();
}

TEST test_findMaxValueOnPlane_slow_z()
{
    PmVector const bounds_easy = {{10.0, 10.0, 1.0}};
    PmVector const l1 = {{1,1,0.1}};
    PmVector const l2 = {{1,-1,-0.1}};

    PmVector u1, u2, u, v, uv_harmonic_sq;
    VecUnit(&l1, &u1);
    VecUnit(&l2, &u2);
    VecVecOrthonormal(&u1, &u2, &u, &v);
    VecVecHarmonicAddSq(&u, &v, &uv_harmonic_sq);

    double b_max;
    findMaxValueOnPlane(&uv_harmonic_sq, &bounds_easy, &b_max);

    ASSERT_IN_RANGE(10.0, b_max, TP_POS_EPSILON);
    PASS();
}

TEST test_findMaxValueOnPlane_tilted()
{
    PmVector const bounds_easy = {{10.0, 10.0, 10.0}};
    PmVector const l1 = {{1,1,1}};
    PmVector const l2 = {{-1,1,0}};

    PmVector u1, u2, u, v, uv_harmonic_sq;
    VecUnit(&l1, &u1);
    VecUnit(&l2, &u2);
    VecVecOrthonormal(&u1, &u2, &u, &v);
    VecVecHarmonicAddSq(&u, &v, &uv_harmonic_sq);

    double b_max;
    findMaxValueOnPlane(&uv_harmonic_sq, &bounds_easy, &b_max);

    ASSERT_IN_RANGE(10.954451150103, b_max, TP_POS_EPSILON);
    PASS();
}

TEST pmCartCartParallel_numerical() {

    PmCartesian u0 = {1,0,0};
    PmCartesian u_close = {cos(TP_ANGLE_EPSILON), sin(TP_ANGLE_EPSILON), 0};

    ASSERT(pmCartCartParallel(&u0, &u0, TP_ANGLE_EPSILON_SQ));
    ASSERT_FALSE(pmCartCartParallel(&u0, &u_close, 0.0));

    // Test that the tolerance makes sense
    ASSERT_FALSE(pmCartCartParallel(&u0, &u_close, 0.5*TP_ANGLE_EPSILON_SQ));
    ASSERT(pmCartCartParallel(&u0, &u_close, 1.5*TP_ANGLE_EPSILON_SQ));

    // Try a bunch of other angles including anti-parallel
    for (double k=1; k <= 7; ++k) {
        PmCartesian u_far = {cos(PM_PI_4 * k), sin(PM_PI_4 * k), 0};
        ASSERT_FALSE(pmCartCartParallel(&u0, &u_far, TP_ANGLE_EPSILON_SQ));
    }

    PASS();
}

TEST pmCartCartAntiParallel_numerical() {

    PmCartesian u0 = {1,0,0};
    PmCartesian u_close = {-cos(TP_ANGLE_EPSILON), sin(TP_ANGLE_EPSILON), 0};

    PmCartesian u_opposite;
    pmCartScalMult(&u0, -1.0, &u_opposite);
    ASSERT(pmCartCartAntiParallel(&u0, &u_opposite, TP_ANGLE_EPSILON_SQ));
    ASSERT_FALSE(pmCartCartAntiParallel(&u0, &u_close, 0.0));

    // Test that the tolerance makes sense
    ASSERT_FALSE(pmCartCartAntiParallel(&u0, &u_close, 0.5*TP_ANGLE_EPSILON_SQ));
    ASSERT(pmCartCartAntiParallel(&u0, &u_close, 1.5*TP_ANGLE_EPSILON_SQ));

    // Try a bunch of other angles including anti-parallel
    for (double k=1; k <= 7; ++k) {
        PmCartesian u_far = {-cos(PM_PI_4 * k), sin(PM_PI_4 * k), 0};
        ASSERT_FALSE(pmCartCartAntiParallel(&u0, &u_far, TP_ANGLE_EPSILON_SQ));
    }

    PASS();
}

SUITE(blendmath) {
    RUN_TEST(test_findAccelScale);
    RUN_TEST(test_findMaxValueOnPlane_basic);
    RUN_TEST(test_findMaxValueOnPlane_missing_y);
    RUN_TEST(test_findMaxValueOnPlane_slow_z);
    RUN_TEST(test_findMaxValueOnPlane_tilted);
    RUN_TEST(pmCartCartParallel_numerical);
    RUN_TEST(pmCartCartAntiParallel_numerical);
}


TEST findMinNonZeroComplete() {
    PmCartesian normal_bounds = {3.0,1.0,2.0};

    double min_bound = findMinNonZero(&normal_bounds);
    ASSERT_EQ(min_bound, 1.0);
    PASS();
}

TEST findMinNonZeroPartial() {
    PmCartesian partial_bounds = {3.0,0.0,2.0};

    double min_bound = findMinNonZero(&partial_bounds);
    ASSERT_EQ(min_bound, 2.0);
    PASS();
}

SUITE(joint_utils) {
    RUN_TEST(findMinNonZeroComplete);
    RUN_TEST(findMinNonZeroPartial);
}


static const double a_max_mock = 10.0;
static const double v_max_mock = 2.0;
static const double cycle_time = 0.001;
static const double target_dist_mock = 2.0;

static const double SMALL_DISTANCE = 1e-10;
static const double SMALL_VELOCITY = 1e-12;

TEST checkEndCondition_zero_velocity()
{
    EndCondition ec_zero = checkEndCondition(cycle_time,
                                   target_dist_mock,
                                   0.0,
                                   0.0,
                                   a_max_mock
                                   );
    // When both initial and final velocity are zero, assume that at least one cycle is left
    ASSERT(ec_zero.dt >= cycle_time);

    EndCondition ec_small = checkEndCondition(cycle_time,
                                   target_dist_mock,
                                   SMALL_VELOCITY,
                                   0.0,
                                   a_max_mock
                                   );
    // when "average" velocity is very small and distance to go is large, at least one cycle is left
    ASSERT(ec_small.dt >= cycle_time);
    PASS();
}
TEST checkEndCondition_small_velocity()
{
    EndCondition ec_small_close = checkEndCondition(cycle_time,
                                   SMALL_VELOCITY*cycle_time / 4.0,
                                   SMALL_VELOCITY,
                                   0.0,
                                   a_max_mock
                                   );
    printf("Found dt %f\n", ec_small_close.dt);
    ASSERT(ec_small_close.dt < cycle_time);

    EndCondition ec_small_far = checkEndCondition(cycle_time,
                                   SMALL_VELOCITY*cycle_time * 2.0,
                                   SMALL_VELOCITY,
                                   0.0,
                                   a_max_mock
                                   );
    // when "average" velocity is very small and distance to go is large, at least one cycle is left
    ASSERT(ec_small_far.dt >= cycle_time);
    PASS();
}


TEST checkEndCondition_at_velocity()
{
    double v_final = 1.0;
    EndCondition ec_close = checkEndCondition(cycle_time,
                                   0.001 - SMALL_DISTANCE,
                                   v_final,
                                   v_final,
                                   a_max_mock
                                   );
    ASSERT(ec_close.dt < cycle_time);
    ASSERT_IN_RANGEm("final velocity", ec_close.v_f, v_final - DOUBLE_FUZZ, v_final + DOUBLE_FUZZ);

    EndCondition ec_far = checkEndCondition(cycle_time,
                                   0.001 + SMALL_DISTANCE,
                                   v_final,
                                   v_final,
                                   a_max_mock
                                   );
    ASSERT(ec_far.dt >= cycle_time);
    ASSERT_IN_RANGEm("final velocity", ec_close.v_f, v_final - DOUBLE_FUZZ, v_final + DOUBLE_FUZZ);
    PASS();
}

TEST checkEndCondition_below_vf_close()
{
    const double v_final = v_max_mock;
    const double v_current = 1.0;
    const double nominal_cycle_dist = v_current * cycle_time;

    EndCondition ec = checkEndCondition(cycle_time,
                                   nominal_cycle_dist,
                                   v_current,
                                   v_final,
                                   a_max_mock
                                   );
    ASSERT(ec.dt < cycle_time);
    ASSERT_IN_RANGEm("final velocity", ec.v_f, 1.0, v_final);
    PASS();
}

TEST checkEndCondition_below_vf_far()
{
    const double v_final = v_max_mock;
    const double v_current = 1.0;
    const double nominal_cycle_dist = v_current * cycle_time;

    EndCondition ec = checkEndCondition(cycle_time,
                                   4.0*nominal_cycle_dist,
                                   v_current,
                                   v_final,
                                   a_max_mock
                                   );
    ASSERT(ec.dt > cycle_time);
    ASSERT_IN_RANGEm("final velocity", ec.v_f, 1.0, v_final);
    PASS();
}

TEST checkEndCondition_above_vf_far()
{
    const double v_final = 1.0;
    const double v_current = v_max_mock;

    EndCondition ec = checkEndCondition(cycle_time,
                                              target_dist_mock,
                                              v_current,
                                              v_final,
                                              a_max_mock
                                              );
    ASSERT(ec.dt > cycle_time);
    ASSERT_IN_RANGEm("final velocity", ec.v_f, 1.0, v_final);
    PASS();
}


TEST checkEndCondition_above_vf_close()
{
    const double v_final = 1.0;
    const double v_current = v_final + a_max_mock * cycle_time;
    const double nominal_cycle_dist = (v_current + v_final) / 2.0 * cycle_time;

    EndCondition ec = checkEndCondition(cycle_time,
                                              nominal_cycle_dist,
                                              v_current,
                                              v_final,
                                              a_max_mock
                                              );
    ASSERT_IN_RANGE(ec.dt, cycle_time, TP_TIME_EPSILON);
    ASSERT_IN_RANGEm("final velocity", ec.v_f, 1.0, v_final);
    PASS();
}


SUITE(tc_functions) {
    RUN_TEST(checkEndCondition_zero_velocity);
    RUN_TEST(checkEndCondition_small_velocity);
    RUN_TEST(checkEndCondition_at_velocity);
    RUN_TEST(checkEndCondition_below_vf_far);
    RUN_TEST(checkEndCondition_below_vf_close);
    RUN_TEST(checkEndCondition_above_vf_far);
    RUN_TEST(checkEndCondition_above_vf_close);
}

TEST test_pmCircleTangentVector_unitcircle()
{
    PmCircle c;
    PmCartesian start = {1,0, 0};
    PmCartesian end = {0,1,0};
    PmCartesian center = {0,0,0};
    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    {
        PmCartesian utan_start;
        const double angle = 0.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_start));
        PmCartesian const expect_utan_start = {0,1,0};
        ASSERT_PMCARTESIAN_IN_RANGE(utan_start, expect_utan_start, CART_FUZZ);
    }

    {
        PmCartesian utan_30deg;
        const double angle = M_PI / 6.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_30deg));
        PmCartesian const expect_utan_30deg = {-0.5, sqrt(3)/2.0,0};
        ASSERT_PMCARTESIAN_IN_RANGE(utan_30deg, expect_utan_30deg, CART_FUZZ);
    }
    {
        PmCartesian utan_60deg;
        const double angle = M_PI / 3.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_60deg));
        PmCartesian const expect_utan_60deg = {-sqrt(3)/2.0, 0.5, 0};
        ASSERT_PMCARTESIAN_IN_RANGE(utan_60deg, expect_utan_60deg, CART_FUZZ);
    }
    {
        PmCartesian utan_end;
        const double angle = M_PI / 2.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_end));
        PmCartesian const expect_utan_end = {-1.0, 0, 0};
        ASSERT_PMCARTESIAN_IN_RANGE(utan_end, expect_utan_end, CART_FUZZ);
    }

    PASS();
}


/**
 * Evaluate the derivative of the ideal XY spiral
 * x(theta) = a*cos(theta) - (r0+a*theta)*sin(theta)
 * y(theta) = a*sin(theta) + (r0+a*theta)*sin(theta)
 *
 * Where a is the spiral component, r0 is radius when theta is zero.
 *
 * Uses the derivative to produce the unit vector at the given theta.
 * From https://math.stackexchange.com/questions/1078185/differentiate-archimedess-spiral
 */
static PmCartesian idealXYSpiralUnitVec(double a, double r0, double theta, double dz)
{
    PmCartesian out = {0};
    out.x = a*cos(theta) - (r0+a*theta)*sin(theta);
    out.y = a*sin(theta) + (r0+a*theta)*cos(theta);
    out.z = dz * theta;
    pmCartUnitEq(&out);
    return out;
}

TEST test_pmCircleTangentVector_spiralout()
{
    PmCircle c;
    const double expect_r0 = 1.0;
    PmCartesian start = {expect_r0,0, 0};
    PmCartesian end = {expect_r0 + M_PI_2,0,0};
    PmCartesian center = {0,0,0};
    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    // a = (r1-r0)/theta = (pi/2) / (2pi) = 1/4
    double expect_spiral_a = 0.25;
    ASSERT_FLOAT_EQ(2.0 * M_PI, c.angle);
    ASSERT_FLOAT_EQ(expect_spiral_a, c.spiral / c.angle);

    {
        PmCartesian utan_start;
        const double angle = 0.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_start));
        PmCartesian const expect_utan_start = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_start, expect_utan_start, CART_FUZZ);
    }

    {
        PmCartesian utan_30deg;
        const double angle = M_PI / 6.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_30deg));
        PmCartesian const expect_utan_30deg = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_30deg, expect_utan_30deg, CART_FUZZ);
    }

    {
        PmCartesian utan_end;
        const double angle = M_PI / 6.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_end));
        PmCartesian const expect_utan_end = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_end, expect_utan_end, CART_FUZZ);
    }

    PASS();
}

TEST test_pmCircleTangentVector_spiralin()
{
    PmCircle c;
    const double expect_r0 = 2.0;
    PmCartesian start = {expect_r0, 0, 0};
    PmCartesian end = {expect_r0 - M_PI_2,0,0};
    PmCartesian center = {0,0,0};
    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 0);

    // a = (r1-r0)/theta = (pi/2) / (2pi) = 1/4
    double expect_spiral_a = -0.25;
    ASSERT_FLOAT_EQ(2.0 * M_PI, c.angle);
    ASSERT_FLOAT_EQ(expect_spiral_a, c.spiral / c.angle);

    {
        PmCartesian utan_start;
        const double angle = 0.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_start));
        PmCartesian const expect_utan_start = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_start, expect_utan_start, CART_FUZZ);
    }

    {
        PmCartesian utan_30deg;
        const double angle = M_PI / 6.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_30deg));
        PmCartesian const expect_utan_30deg = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_30deg, expect_utan_30deg, CART_FUZZ);
    }

    {
        PmCartesian utan_end;
        const double angle = M_PI / 6.0;
        ASSERT_FALSE(pmCircleTangentVector(&c, angle, &utan_end));
        PmCartesian const expect_utan_end = idealXYSpiralUnitVec(expect_spiral_a, expect_r0, angle, 0);
        ASSERT_PMCARTESIAN_IN_RANGE(utan_end, expect_utan_end, CART_FUZZ);
    }

    PASS();
}


TEST test_findSpiralArcLengthFit_spiralout()
{
    // Unit circle with some spiral component centered at origin
    PmCircle c = {0};
    c.rTan.x = 1;
    c.rPerp.y = 1;
    c.angle = M_PI;
    c.spiral = 0.001;
    c.normal.z = 1;
    c.radius = 1;

    SpiralArcLengthFit f;
    findSpiralArcLengthFit(&c, &f, 1e-6);

    ASSERT_FALSE(f.spiral_in);
    ASSERT(f.total_planar_length > c.radius * c.angle);
    ASSERT(f.b0 > 0);
    ASSERT(f.b1 > 0);
    PASS();
}

TEST test_findSpiralArcLengthFit_spiralin()
{
    // Unit circle with some spiral component centered at origin
    PmCircle c = {0};
    c.rTan.x = 1;
    c.rPerp.y = 1;
    c.angle = M_PI;
    c.spiral = -0.1;
    c.normal.z = 1;
    c.radius = 1;

    SpiralArcLengthFit f;
    findSpiralArcLengthFit(&c, &f, 1e-6);

    ASSERT(f.spiral_in);
    ASSERT(f.total_planar_length < c.radius * c.angle);
    ASSERT(f.b0 > 0);
    ASSERT(f.b1 > 0);
    PASS();
}


TEST test_findSpiralArcLengthFit_exact()
{
    // Unit circle with some spiral component centered at origin
    PmCircle c = {0};
    c.rTan.x = 1;
    c.rPerp.y = 1;
    c.angle = M_PI;
    c.spiral = 0.0;
    c.normal.z = 1;
    c.radius = 1;

    SpiralArcLengthFit f;
    findSpiralArcLengthFit(&c, &f, 1e-6);

    ASSERT_FALSE(f.spiral_in);
    ASSERT_FLOAT_EQ(c.radius * c.angle, f.total_planar_length);
    ASSERT_FLOAT_EQ(0, f.b0);
    ASSERT_FLOAT_EQ(1, f.b1);
    PASS();
}

TEST test_pmCircleActualMaxVel_cutoff()
{
    double const v_max = 8.0;
    double const a_max = 30.0;
    double const r_cutoff = pmSq(v_max)/(a_max * BLEND_ACC_RATIO_NORMAL);

    PmCircle c;
    PmCartesian start = {r_cutoff,0, 0};
    PmCartesian end = start;
    PmCartesian center = {0,0,0};
    PmCartesian normal = {0,0,1};
    pmCircleInit(&c, &start, &end, &center, &normal, 2);

    PmCircleLimits lim = pmCircleActualMaxVel(&c, v_max, a_max);

    ASSERT_IN_RANGE(lim.v_max, v_max, CART_FUZZ);
    ASSERT_IN_RANGE(lim.acc_ratio, 0.5, CART_FUZZ);
    PASS();
}

TEST test_pmCircle9Init()
{
    PmCircle9 c;

    PmVector start = {{1.0, 2.0, 3.0, 4, 5, 6, 7, 8, 9}};
    PmVector end = {{3.0, 2.0, 4.0, 5, 4, 3, 2, 1, 0}};
    PmCartesian center = {2.0, 2.0, 3.0};
    PmCartesian normal = {0, 0, 1};

    ASSERT_FALSE(pmCircle9Init(&c, &start, &end, &center, &normal, 0));
    ASSERT_PMCARTESIAN_IN_RANGE(c.xyz.center, center, CART_FUZZ);
    ASSERT_FLOAT_EQ(c.xyz.radius, 1.0);
    ASSERT_IN_RANGE(c.xyz.spiral, 0.0, TP_POS_EPSILON);

    double target = pmCircle9Length(&c);
    double expected_target = sqrt(pmSq(M_PI*1.0) + pmSq(1.0));
    ASSERT_IN_RANGE(expected_target, target, TP_POS_EPSILON);
    PASS();
}

TEST test_pmCircle9Target()
{
    PmCircle9 c;
    PmVector start = {{1.0, 2.0, 3.0,0,0,0,0,0,0}};
    PmVector end = start;
    end.ax[0] = 3.5;
    end.ax[2] = 7.0;
    PmCartesian center = {2.0, 2.0, 3.0};

    PmCartesian normal = {0, 0, 1};

    ASSERT_FALSE(pmCircle9Init(&c, &start, &end, &center, &normal, 0));

    double target = pmCircle9Length(&c);
    // KLUDGE get known good value from current code (assumes spiral fit is close enough)
    // FIXME likely to break with small changes in fit strategy
    double expected_target = 5.628631592039345;
    ASSERT_IN_RANGE(expected_target, target, TP_POS_EPSILON);

    PASS();
}

SUITE(circle_funcs)
{
    RUN_TEST(test_pmCircleTangentVector_unitcircle);
    RUN_TEST(test_pmCircleTangentVector_spiralout);
    RUN_TEST(test_pmCircleTangentVector_spiralin);
    RUN_TEST(test_findSpiralArcLengthFit_spiralout);
    RUN_TEST(test_findSpiralArcLengthFit_spiralin);
    RUN_TEST(test_findSpiralArcLengthFit_exact);
    RUN_TEST(test_pmCircleActualMaxVel_cutoff);
    RUN_TEST(test_pmCircle9Init);
    RUN_TEST(test_pmCircle9Target);
}

static int checkQF(double A, double B, double C, double exp_R0, double exp_R1, double tol)
{
    double r0=0, r1=0;

    ASSERT(0 == quadraticFormula(A, B, C, &r0, &r1));
    ASSERT_IN_RANGE(exp_R0, r0, tol);
    ASSERT_IN_RANGE(exp_R1, r1, tol);
    PASS();
}

TEST test_quadratic_formula_basic()
{
    // Simple cases
    // 0 = Ax^2 + Bx + C
    CHECK_CALL(checkQF(1, -4, 4, 2, 2, DOUBLE_FUZZ));
    CHECK_CALL(checkQF(1, 4, 4, -2, -2, DOUBLE_FUZZ));
    // Expect the most-positive root first
    CHECK_CALL(checkQF(1, 1, -6, 2, -3, DOUBLE_FUZZ));
    // Expect small roots to be calculated precisely
    CHECK_CALL(checkQF(1, 1.0000000000000001, 1e-16, -1e-16, -1, DOUBLE_FUZZ));

    PASS();
}

TEST test_find_blend_points_and_tangents()
{
    PASS();
}

TEST test_find_blend_intermediate_segments()
{
    PASS();
}

SUITE(biarc_optimization) {
    RUN_TEST(test_find_blend_points_and_tangents);
    RUN_TEST(test_find_blend_intermediate_segments);
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_SUITE(blendmath);
    RUN_SUITE(joint_utils);
    RUN_SUITE(tc_functions);
    RUN_SUITE(circle_funcs);
    RUN_TEST(test_quadratic_formula_basic);
    RUN_SUITE(biarc_optimization);
    GREATEST_MAIN_END();        /* display results */
}
