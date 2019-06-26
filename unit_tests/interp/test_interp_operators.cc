#include "catch.hpp"

#include <interp_testing_util.hh> // For core interp stuff and extra REQUIRE macros/ setup
#include <rs274ngc_interp.hh>
#include <interp_inspection.hh>
#include <interp_return.hh>
#include <interp_parameter_def.hh>
using namespace interp_param_global;

TEST_CASE("Interp Unary Operators")
{
  DECL_INIT_TEST_INTERP();

  SECTION("Convert input units to program mm")
  {
    REQUIRE_INTERP_OK(execute(test_interp,"G21"));
    REQUIRE_INTERP_OK(execute(test_interp,"#1=[U_INCH[1.0]]"));
    CHECK_FUZZ(settings->parameters[1], 25.4);
    REQUIRE_INTERP_OK(execute(test_interp,"#2=[U_MM[25.4]]"));
    CHECK_FUZZ(settings->parameters[2], 25.4);
  }
  SECTION("Convert input units to program inches")
  {
    REQUIRE_INTERP_OK(execute(test_interp,"G20"));
    REQUIRE_INTERP_OK(execute(test_interp,"#3=[U_INCH[1.0]]"));
    CHECK_FUZZ(settings->parameters[3], 1.0);
    REQUIRE_INTERP_OK(execute(test_interp,"#4=[U_MM[25.4]]"));
    CHECK_FUZZ(settings->parameters[4], 1.0);
  }
}

SCENARIO("Interp Non-modal incremental words G90.2 and G91.2")
{
  DECL_INIT_TEST_INTERP();
  GIVEN("Machine is in default state")
  {
    THEN("Multiple non-modal commands should be an error")
    {
      REQUIRE_INTERP_ERROR(test_interp.execute("G53 G4 G0 X1"));
      // Test other combinations here
    }
  }

  GIVEN("Machine is currently in absolute mode")
  {
    REQUIRE_INTERP_OK(execute(test_interp, "G20 G90"));
    REQUIRE_INTERP_OK(execute(test_interp, "G0 X10 Y11 Z12"));

    THEN ("A single line with G91.2 should be treated as incremental, but subsequent lines as absolute")
    {
      CHECK_FUZZ(currentX(settings), 10.0);
      CHECK_FUZZ(currentY(settings), 11.0);
      CHECK_FUZZ(currentZ(settings), 12.0);
      REQUIRE_INTERP_OK(execute(test_interp, "G91.2 G0 X1 Y2 Z3"));
      CHECK_FUZZ(currentX(settings), 11.0);
      CHECK_FUZZ(currentY(settings), 13.0);
      CHECK_FUZZ(currentZ(settings), 15.0);
      REQUIRE_INTERP_OK(execute(test_interp, "G0 X2 Y-4 Z6"));
      CHECK_FUZZ(currentX(settings), 2.0);
      CHECK_FUZZ(currentY(settings), -4.0);
      CHECK_FUZZ(currentZ(settings), 6.0);
    }

    THEN ("G90.2 should have no effect")
    {
      REQUIRE_INTERP_OK(execute(test_interp, "G90.2 G0 X1 Y2 Z3"));
      CHECK_FUZZ(currentX(settings), 1.0);
      CHECK_FUZZ(currentY(settings), 2.0);
      CHECK_FUZZ(currentZ(settings), 3.0);
    }
  }

  GIVEN("Machine in incremental mode")
  {
    REQUIRE_INTERP_OK(execute(test_interp, "G20 G90"));
    REQUIRE_INTERP_OK(execute(test_interp, "G0 X-2 Y-4 Z6"));
    REQUIRE_INTERP_OK(execute(test_interp, "G91"));
    CHECK_FUZZ(currentX(settings), -2.0);
    CHECK_FUZZ(currentY(settings), -4.0);
    CHECK_FUZZ(currentZ(settings), 6.0);

    THEN ("A single line with G90.2 should be treated as absolute, but subsequent lines as incremental")
    {
      REQUIRE_INTERP_OK(execute(test_interp, "G90.2 G0 X1 Y2 Z3"));
      CHECK_FUZZ(currentX(settings), 1.0);
      CHECK_FUZZ(currentY(settings), 2.0);
      CHECK_FUZZ(currentZ(settings), 3.0);
      REQUIRE_INTERP_OK(execute(test_interp, "G0 X2 Y-4 Z6"));
      CHECK_FUZZ(currentX(settings), 3.0);
      CHECK_FUZZ(currentY(settings), -2.0);
      CHECK_FUZZ(currentZ(settings), 9.0);
    }

    THEN("G91.2 should have no effect") {
      REQUIRE_INTERP_OK(execute(test_interp, "G91.2 G0 X-3 Y-4 Z-5"));
      CHECK_FUZZ(currentX(settings), -5.0);
      CHECK_FUZZ(currentY(settings), -8.0);
      CHECK_FUZZ(currentZ(settings), 1.0);
      REQUIRE_INTERP_OK(execute(test_interp, "G0 X2 Y3 Z1"));
      CHECK_FUZZ(currentX(settings), -3.0);
      CHECK_FUZZ(currentY(settings), -5.0);
      CHECK_FUZZ(currentZ(settings), 2.0);
    }
  }

}
