#include "pm_line9.h"
#include "greatest.h"
#include "rtapi.h"
#include "math.h"
#include "motion_debug.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST test_pmLine9Init()
{
    PmLine9 line9;
    PmVector start = {0};
    PmVector end = {{1,2,3,4,5,6,7,8,9}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end));
    ASSERT_PMVECTOR_IN_RANGE(end, line9.end, 1e-16);
    ASSERT_IN_RANGE(1.0, VecMag(&line9.uVec), 1e-16);
    PASS();
}

TEST test_pmLine9Point()
{
    PmLine9 line9;
    PmVector start = {0};
    PmVector end = {{1,2,3,4,5,6,7,8,9}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end));

    double len = pmLine9Length(&line9);
    PmVector gotv={0};
    pmLine9Point(&line9, len, &gotv);
    ASSERT_PMVECTOR_IN_RANGE(end, gotv, 1e-16);
    pmLine9Point(&line9, 0, &gotv);
    ASSERT_PMVECTOR_IN_RANGE(start, gotv, 1e-16);

    PASS();
}

TEST test_pmLine9Stretch_start()
{
    PmLine9 line9;
    PmVector start = {0};
    PmVector end = {{1,2,3,4,5,6,7,8,9}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end));

    double cut_len = 0.375;
    PmVector cut_pt={0};
    pmLine9Point(&line9, cut_len, &cut_pt);

    PmLine9 line9_cut = line9;
    pmLine9Cut(&line9_cut, cut_len, KEEP_START_PT);
    PmVector cut_end;
    pmLine9Point(&line9_cut, cut_len, &cut_end);
    ASSERT_PMVECTOR_IN_RANGE(cut_pt, cut_end, 1e-15);

    PASS();
}

TEST test_pmLine9Stretch_end()
{
    PmLine9 line9;
    PmVector start = {0};
    PmVector end = {{1,2,3,4,5,6,7,8,9}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end));

    double cut_len = 0.875;
    PmVector cut_pt={0};
    pmLine9Point(&line9, cut_len, &cut_pt);

    PmLine9 line9_cut = line9;
    pmLine9Cut(&line9_cut, cut_len, KEEP_END_PT);
    PmVector cut_start;
    pmLine9Point(&line9_cut, 0, &cut_start);
    ASSERT_PMVECTOR_IN_RANGE(cut_pt, cut_start, 1e-15);

    PASS();
}

TEST test_pmLine9VLimit()
{
    PmLine9 line9;
    const double z_height = 3;
    PmVector const start = {0};
    PmVector const end_xyz = {{3,3,z_height,0,0,0,0,0,0}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end_xyz));
    ASSERT_FLOAT_EQ(4.0, pmLine9VLimit(&line9, 100.0, 4.0, 2.0));
    PmVector const end_abc = {{0,0,0,1,2,1,0,0,0}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end_abc));
    ASSERT_FLOAT_EQ(2.0, pmLine9VLimit(&line9, 100.0, 4.0, 2.0));
    PmVector const end_all = {{1,0,0,0,1,0,1,0,0}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end_all));
    ASSERT_FLOAT_EQ(sqrt(12), pmLine9VLimit(&line9, 100.0, 4.0, 2.0));
    ASSERT_FLOAT_EQ(3.0, pmLine9VLimit(&line9, 3.0, 4.0, 2.0));
    PASS();
}


int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(test_pmLine9Init);
    RUN_TEST(test_pmLine9Point);
    RUN_TEST(test_pmLine9Stretch_start);
    RUN_TEST(test_pmLine9Stretch_end);
    RUN_TEST(test_pmLine9VLimit);
    GREATEST_MAIN_END();
}
