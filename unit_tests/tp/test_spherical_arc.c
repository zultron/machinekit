#include "tp_debug.h"
#include "spherical_arc.h"
#include "spherical_arc9.h"
#include "greatest.h"
#include "blendmath.h"
#include "tp_types.h"
#include "math.h"
#include "rtapi.h"
#include "motion_debug.h"
#include "greatest_compare.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

TEST arcInitFromPoints_simple() {
    SphericalArc arc;
    PmCartesian start = {1,2,0};
    PmCartesian end = {2,3,0};
    PmCartesian center = {1,3,0};
    PmCartesian uTan = {0,0,0};

    int res = arcInitFromPoints(&arc, &start, &end, &center, &uTan, 0);
    ASSERT_FALSE(res);

    ASSERT_PMCARTESIAN_IN_RANGE(arc.end, end, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(arc.start, start, CART_FUZZ);
    ASSERT_PMCARTESIAN_IN_RANGE(arc.center, center, CART_FUZZ);

    double mag;
    pmCartMag(&arc.rStart, &mag);
    ASSERT_FLOAT_EQ(mag, 1.0);
    pmCartMag(&arc.rEnd, &mag);
    ASSERT_FLOAT_EQ(mag, 1.0);

    ASSERT_FLOAT_EQ(arc.radius, 1.0);
    ASSERT_FLOAT_EQ(arc.Sangle, 1.0);
    ASSERT_FLOAT_EQ(arc.spiral, 0.0);
    ASSERT_FLOAT_EQ(arc.angle, M_PI_2);

    PmCartesian pt={0};
    arcPoint(&arc, 0, &pt);
    ASSERT_PMCARTESIAN_IN_RANGE(arc.center, center, CART_FUZZ);

    PASS();
}


TEST test_arc9InitFromPoints() {
    //TODO
    PASS();
}

TEST test_arc9Point() {
    //TODO
    PASS();
}

TEST test_arc9Length() {
    //TODO
    PASS();
}

TEST test_arc9Tangent()
{
    //TODO
    PASS();
}


int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(arcInitFromPoints_simple);
    RUN_TEST(test_arc9InitFromPoints);
    RUN_TEST(test_arc9Point);
    RUN_TEST(test_arc9Length);
    RUN_TEST(test_arc9Tangent);
    GREATEST_MAIN_END();
}
