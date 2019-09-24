#ifndef INTERP_TESTING_UTIL_HH
#define INTERP_TESTING_UTIL_HH

#include <stddef.h>
#include <rs274ngc_interp.hh>
#include <interp_return.hh>
#include <catch.hpp>
#include <mock_xyza_config.hh>
#include <cstdio>

using Catch::Matchers::WithinAbs;
#define INTERP_FUZZ 1e-10

#define INTERP_EXECUTE(interp__, cmd__) execute_(interp__, cmd__, __FILE__, __LINE__)

#define REQUIRE_FUZZ(got, expected) REQUIRE_THAT(got, WithinAbs(expected, INTERP_FUZZ))
#define CHECK_FUZZ(got, expected) CHECK_THAT(got, WithinAbs(expected, INTERP_FUZZ))

#define REQUIRE_INTERP_OK(val) REQUIRE(val < INTERP_MIN_ERROR)
#define CHECK_INTERP_OK(val) CHECK(val < INTERP_MIN_ERROR)
#define REQUIRE_INTERP_ERROR(val) REQUIRE(val >= INTERP_MIN_ERROR)
#define CHECK_INTERP_ERROR(val) CHECK(val >= INTERP_MIN_ERROR)

#define REQUIRE_INTERP_EXEC_OK(i, cmd) REQUIRE_INTERP_OK(INTERP_EXECUTE(i, cmd))
#define REQUIRE_INTERP_EXEC_ERROR(i, cmd) REQUIRE_INTERP_ERROR(INTERP_EXECUTE(i, cmd))

extern InterpBase *pinterp;
extern int emc_debug;

int print_interp_error(Interp &interp, int retval);

int execute_(Interp &interp, const char *cmd, const char *src_file, int line);
int execute(Interp &interp, const char *cmd);

template <size_t N>
int execute_lines_or_err(Interp &interp, const char* (&lines)[N] )
{
  for (auto l : lines) {
      auto res = (InterpReturn) execute(interp, l);
      if (isInterpError(res)) {
          return res;
      }
  }
  return INTERP_OK;
}

// Do a bunch of setup / assignments to force reinitialize SAI internals enough that sections won't step on each other
#define DECL_INIT_TEST_INTERP() \
    setup_xyza_config(); \
    SET_PARAMETER_FILE_NAME("unit_test.var"); \
    std::remove("unit_test.var"); \
    Interp test_interp = Interp(); \
    pinterp = &test_interp; \
    setup_struct * const settings = &test_interp._setup; \
    test_interp.init(); \
    settings->tool_table[0].toolno = 1

#endif // INTERP_TESTING_UTIL_HH
