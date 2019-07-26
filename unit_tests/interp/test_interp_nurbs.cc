#include "catch.hpp"

#include <interp_testing_util.hh> // For core interp stuff and extra REQUIRE macros/ setup
#include <rs274ngc_interp.hh>
#include <interp_inspection.hh>
#include <interp_return.hh>
#include <interp_parameter_def.hh>
using namespace interp_param_global;


SCENARIO("nurbs move commanded with various non-modal commands")
{
    DECL_INIT_TEST_INTERP();
    GIVEN("Machine is in default state with zero initial position and offsets")
    {
        REQUIRE_INTERP_OK(execute(test_interp, "G90 G20 G0 X1 Y1 Z1 F20"));
        REQUIRE_INTERP_OK(execute(test_interp, "G10 L2 P0 X0 Y0 Z0"));
        THEN("NURBS command should produce a nice example curve")
        {
            REQUIRE_INTERP_OK(execute(test_interp, "G5.2 P1 L3"));
            REQUIRE_INTERP_OK(execute(test_interp, "X0 Y1 P1"));
            REQUIRE_INTERP_OK(execute(test_interp, "X2 Y2 P1"));
            REQUIRE_INTERP_OK(execute(test_interp, "X2 Y0 P1"));
            REQUIRE_INTERP_OK(execute(test_interp, "X0 Y0 P1"));
            REQUIRE_INTERP_OK(execute(test_interp, "X0 Y0 P2"));
            REQUIRE_INTERP_OK(execute(test_interp, "G5.3"));
        }
        GIVEN("G53 is specified during a G5 NURBS command")
        {
            THEN("Command should be allowed")
            {
                REQUIRE_INTERP_OK(execute(test_interp, "G5.2 P1 L3"));
                REQUIRE_INTERP_OK(execute(test_interp, "X0 Y1 P1"));
                REQUIRE_INTERP_OK(execute(test_interp, "G53 X2 Y2 P1"));
                REQUIRE_INTERP_OK(execute(test_interp, "X2 Y0 P1"));
                REQUIRE_INTERP_OK(execute(test_interp, "X0 Y0 P1"));
                REQUIRE_INTERP_OK(execute(test_interp, "X0 Y0 P2"));
                REQUIRE_INTERP_OK(execute(test_interp, "G5.3"));
                CHECK_FUZZ(currentX(settings), 0.0);
                CHECK_FUZZ(currentY(settings), 0.0);
            }
        }
    }
}
