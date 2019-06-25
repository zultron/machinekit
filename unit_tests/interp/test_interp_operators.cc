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
    REQUIRE_INTERP_OK(test_interp.execute("G21"));
    REQUIRE_INTERP_OK(test_interp.execute("#1=[U_INCH[1.0]]"));
    CHECK_FUZZ(settings->parameters[1], 25.4);
    REQUIRE_INTERP_OK(test_interp.execute("#2=[U_MM[25.4]]"));
    CHECK_FUZZ(settings->parameters[2], 25.4);
  }
  SECTION("Convert input units to program inches")
  {
    REQUIRE_INTERP_OK(test_interp.execute("G20"));
    REQUIRE_INTERP_OK(test_interp.execute("#3=[U_INCH[1.0]]"));
    CHECK_FUZZ(settings->parameters[3], 1.0);
    REQUIRE_INTERP_OK(test_interp.execute("#4=[U_MM[25.4]]"));
    CHECK_FUZZ(settings->parameters[4], 1.0);
  }
}
