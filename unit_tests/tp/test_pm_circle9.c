#include "pm_circle9.h"
#include "greatest.h"
#include "rtapi.h"
#include "math.h"
#include "motion_debug.h"
#include "greatest_compare.h"
#include "math_util.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"
static PmVector const start_0 = {0};
static PmVector const end_simple = {{1,1, M_PI_2, M_PI_2,0,0,0,0,0}};
static PmCartesian const center_y = {0, 1, 0};
static PmCartesian const normal_z = {0, 0, 1};

TEST test_pmCircle9Init()
{
    PmCircle9 circ9;
    const double z_height = 0;
    PmVector const end = {{1,1,z_height,4,5,6,7,8,9}};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    const double exp_planar_length = M_PI_2;
    const double exp_length_abc = pmSqrt(pmSq(4)+pmSq(5)+pmSq(6));
    const double exp_length_uvw = pmSqrt(pmSq(7)+pmSq(8)+pmSq(9));
    const double exp_length = hypot4(exp_planar_length, z_height, exp_length_abc, exp_length_uvw);
    const double exp_length_ratio_xyz = hypot2(exp_planar_length, z_height) / exp_length;
    const double exp_length_ratio_abc = exp_length_abc / exp_length;
    const double exp_length_ratio_uvw = exp_length_uvw / exp_length;
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end, &center, &normal, 0));
    ASSERT_FLOAT_EQ(exp_planar_length, circ9.fit.total_planar_length);
    ASSERT_FLOAT_EQ(exp_length, pmCircle9Length(&circ9));

    ASSERT_FLOAT_EQ(exp_length_ratio_xyz, circ9.xyz_ratio);
    ASSERT_FLOAT_EQ(exp_length_ratio_abc, circ9.abc_ratio);
    ASSERT_FLOAT_EQ(exp_length_ratio_uvw, circ9.uvw_ratio);
    PASS();
}

TEST test_pmCircle9Point_ends()
{
    PmCircle9 circ9;
    PmVector end = {{1,2,3,0.4,0.5,0.6,0.7,0.8,0.9}};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end, &center, &normal, 0));

    PmVector got_start;
    pmCircle9Point(&circ9, 0, &got_start);
    ASSERT_PMVECTOR_IN_RANGE(start_0, got_start, 1e-16);

    PmVector got_end;
    pmCircle9Point(&circ9, pmCircle9Length(&circ9), &got_end);

    ASSERT_PMVECTOR_IN_RANGE(end, got_end, 1e-15);
    PASS();
}

TEST test_pmCircle9Point_mid()
{
    PmCircle9 circ9;
    PmVector end = {{1,1, M_PI_2, M_PI_2,0,0,0,0,0}};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end, &center, &normal, 0));

    PmVector got_mid;
    pmCircle9Point(&circ9, pmCircle9Length(&circ9)/2.0, &got_mid);

    PmVector exp_mid = {{cos(M_PI_4), 1-sin(M_PI_4), M_PI_4, M_PI_4}};
    ASSERT_PMVECTOR_IN_RANGE(exp_mid, got_mid, 1e-15);
    PASS();
}

static PmVector numericalTangent(PmCircle9 const * const circ9, double progress, double dp)
{
    PmVector pt0, pt1, out;
    pmCircle9Point(circ9, progress, &pt0);
    pmCircle9Point(circ9, progress + dp, &pt1);
    VecVecSub(&pt1, &pt0, &out);
    VecScalMultEq(&out, 1./dp);
    return out;
}

TEST test_pmCircle9TangentVector()
{
    PmCircle9 circ9;
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end_simple, &center_y, &normal_z, 0));

    PmVector exp_tangent = {{1,0,1,0,0,0,0,0,0}};
    ASSERT_FALSE(VecUnitEq(&exp_tangent));

    PmVector numerical_tangent1 = numericalTangent(&circ9, 0, 0.00000001);
    PmVector circ_tangent1;
    pmCircle9TangentVector(&circ9, 0, &circ_tangent1);
    ASSERT_PMVECTOR_IN_RANGE(numerical_tangent1, circ_tangent1, 1e-12);

    PmVector end2 = {{1,1, M_PI_2, M_PI_2,0,0,0,0,0}};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end2, &center_y, &normal_z, 0));

    PmVector numerical_tangent2 = numericalTangent(&circ9, 0, 0.00000001);
    PmVector circ_tangent2;
    pmCircle9TangentVector(&circ9, 0, &circ_tangent2);
    ASSERT_PMVECTOR_IN_RANGE(numerical_tangent2, circ_tangent2, 1e-12);

    PASS();
}

TEST test_pmCircle9Stretch_from_start()
{
    PmCircle9 circ9;
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end_simple, &center_y, &normal_z, 0));

    PmVector mid_pt;
    double orig_len = pmCircle9Length(&circ9);
    pmCircle9Point(&circ9, orig_len / 2.0, &mid_pt);

    PmCircle9 circ9_stretch_from_start = circ9;
    pmCircle9Cut(&circ9_stretch_from_start, orig_len / 2.0, 0);

    PmVector end_pt1;
    pmCircle9Point(&circ9, orig_len / 2.0, &end_pt1);
    ASSERT_PMVECTOR_IN_RANGE(mid_pt, end_pt1, 1e-15);

    PASS();
}

TEST test_pmCircle9Stretch_from_end()
{
    PmCircle9 circ9;
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end_simple, &center_y, &normal_z, 0));

    PmVector mid_pt;
    double orig_len = pmCircle9Length(&circ9);
    ASSERT_FALSE(pmCircle9Point(&circ9, orig_len / 2.0, &mid_pt));

    PmCircle9 circ9_stretch_from_start = circ9;
    pmCircle9Cut(&circ9_stretch_from_start, orig_len / 2.0, 1);

    PmVector start_pt;
    pmCircle9Point(&circ9_stretch_from_start, 0, &start_pt);
    ASSERT_PMVECTOR_IN_RANGE(mid_pt, start_pt, 1e-15);

    PASS();
}

TEST test_pmCircle9VLimit()
{
    PmCircle9 circ9;
    const double z_height = 3;
    PmVector const end = {{3,3,z_height,0,0,0,0,0,0}};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start_0, &end, &center, &normal, 0));
    ASSERT_FLOAT_EQ(4.0, pmCircle9VLimit(&circ9, 5.0, 4.0, 2.0));
    PASS();
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(test_pmCircle9Init);
    RUN_TEST(test_pmCircle9Point_ends);
    RUN_TEST(test_pmCircle9Point_mid);
    RUN_TEST(test_pmCircle9TangentVector);
    RUN_TEST(test_pmCircle9Stretch_from_start);
    RUN_TEST(test_pmCircle9Stretch_from_end);
    RUN_TEST(test_pmCircle9VLimit);
    GREATEST_MAIN_END();
}
