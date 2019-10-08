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
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end, false));
    ASSERT_PMVECTOR_IN_RANGE(end, line9.end, 1e-16);
    ASSERT_IN_RANGE(1.0, VecMag(&line9.uVec), 1e-16);
    PASS();
}

TEST test_pmLine9Point()
{
    PmLine9 line9;
    PmVector start = {0};
    PmVector end = {{1,2,3,4,5,6,7,8,9}};
    ASSERT_FALSE(pmLine9Init(&line9, &start, &end, false));

    double len = pmLine9Length(&line9);
    PmVector gotv={0};
    pmLine9Point(&line9, len, &gotv);
    ASSERT_PMVECTOR_IN_RANGE(end, gotv, 1e-16);
    pmLine9Point(&line9, 0, &gotv);
    ASSERT_PMVECTOR_IN_RANGE(start, gotv, 1e-16);

    PASS();
}


int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(test_pmLine9Init);
    RUN_TEST(test_pmLine9Point);
    GREATEST_MAIN_END();
}
