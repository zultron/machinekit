#include "pm_circle9.h"
#include "greatest.h"
#include "rtapi.h"
#include "math.h"
#include "motion_debug.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST test_pmCircle9Init()
{
    PmCircle9 circ9;
    PmVector start = {0};
    PmVector end = {1,2,3,4,5,6,7,8,9};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start, &end, &center, &normal, 0));
    PASS();
}

TEST test_pmCircle9Point()
{
    PmCircle9 circ9;
    PmVector start = {0};
    PmVector end = {1,2,3,0.4,0.5,0.6,0.7,0.8,0.9};
    PmCartesian center = {0, 1, 0};
    PmCartesian normal = {0, 0, 1};
    ASSERT_FALSE(pmCircle9Init(&circ9, &start, &end, &center, &normal, 0));

    PmVector got_start;
    pmCircle9Point(&circ9, 0, &got_start);
    ASSERT_PMVECTOR_IN_RANGE(start, got_start, 1e-16);

    PmVector got_end;
    pmCircle9Point(&circ9, pmCircle9Length(&circ9), &got_end);

    ASSERT_PMVECTOR_IN_RANGE(end, got_end, 1e-15);
    PASS();
}

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(test_pmCircle9Init);
    RUN_TEST(test_pmCircle9Point);
    GREATEST_MAIN_END();
}
