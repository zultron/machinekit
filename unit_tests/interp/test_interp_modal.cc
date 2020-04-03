#include "catch.hpp"

#include <interp_testing_util.hh> // For core interp stuff and extra REQUIRE macros/ setup
#include <rs274ngc_interp.hh>
#include <interp_inspection.hh>
#include <interp_return.hh>
#include <interp_parameter_def.hh>
#include <sstream>
#include <cstdio>
using namespace interp_param_global;

SCENARIO("G17, G18, G19 Plane selection")
{
    DECL_INIT_TEST_INTERP();
    GIVEN("G17 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g17"));
        CHECK(test_interp._setup.plane == CANON_PLANE_XY);
    }
    GIVEN("G18 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g18"));
        CHECK(test_interp._setup.plane == CANON_PLANE_XZ);
    }
    GIVEN("G19 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g19"));
        CHECK(test_interp._setup.plane == CANON_PLANE_YZ);
    }
    GIVEN("G17.1 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g17.1"));
        CHECK(test_interp._setup.plane == CANON_PLANE_UV);
    }
    GIVEN("G18.1 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g18.1"));
        CHECK(test_interp._setup.plane == CANON_PLANE_UW);
    }
    GIVEN("G19.1 called") {
        REQUIRE_INTERP_OK(test_interp, execute("g19.1"));
        CHECK(test_interp._setup.plane == CANON_PLANE_VW);
    }
}
