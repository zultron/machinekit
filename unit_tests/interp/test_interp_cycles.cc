#include "catch.hpp"

#include <interp_testing_util.hh> // For core interp stuff and extra REQUIRE macros/ setup
#include <rs274ngc_interp.hh>
#include <interp_inspection.hh>
#include <interp_return.hh>
#include <interp_parameter_def.hh>
#include <sstream>
#include <cstdio>
using namespace interp_param_global;

// TODO way more unit tests of cycles here

SCENARIO("test G84.1 low-level tapping cycle")
{
    DECL_INIT_TEST_INTERP();

    const char *setup_lines[] = {
        "g98 ; retract to previous Z mode",
        "g90 ; absolute coordinates mode",
        "g20 ; imperial",
        };
    REQUIRE_INTERP_EXEC_LINES_OK(test_interp, setup_lines);

    GIVEN ("G17 plane and an initial position") {
        const char *g17_lines[] = {
            "g17 ; default plane",
            "g0 x0 y0 z2",
            "f20",
            "M3 S1000",
            };
        REQUIRE_INTERP_EXEC_LINES_OK(test_interp, g17_lines);
        THEN("G84 cycles in g17") {
            // There are six possible orderings of Z, OLD_Z, and R,
            // but the three that have R < Z are rejected by LinuxCNC.
            CHECK_INTERP_OK(test_interp, execute("g84.1 x1 y1 z-1 r1 p0.0"));
            CHECK_INTERP_OK(test_interp, execute("g84.1 x1 y2 z0.5 r3 p0.0"));
            CHECK_INTERP_OK(test_interp, execute("g84.1 x1 y3 z4 r5 p0.0"));
            CHECK_INTERP_ERROR(test_interp, execute("g84.1 x1 y1 z-1 r-2 p0.0"));
            CHECK_INTERP_ERROR(test_interp, execute("g84.1 x1 y2 z0.5 r0.2 p0.0"));
            CHECK_INTERP_ERROR(test_interp, execute("g84.1 x1 y3 z4 r3 p0.0"));
        }
    }
}
