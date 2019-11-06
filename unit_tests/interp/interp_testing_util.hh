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

#define REQUIRE_FUZZ(got, expected) REQUIRE_THAT(got, WithinAbs(expected, INTERP_FUZZ))
#define CHECK_FUZZ(got, expected) CHECK_THAT(got, WithinAbs(expected, INTERP_FUZZ))

#define REQUIRE_INTERP_OK(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) < INTERP_MIN_ERROR)
#define CHECK_INTERP_OK(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) < INTERP_MIN_ERROR)
#define REQUIRE_INTERP_ERROR(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) >= INTERP_MIN_ERROR)
#define CHECK_INTERP_ERROR(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) >= INTERP_MIN_ERROR)

#define REQUIRE_INTERP_EXEC_LINES_OK(interp, lines) REQUIRE(execute_lines_or_err(interp, lines, __FILE__, __LINE__) < INTERP_MIN_ERROR)

extern InterpBase *pinterp;
extern int emc_debug;

int print_interp_error(Interp &interp, int retval);
int print_interp_error(Interp & interp, int retval, const char* src_file, int line);

template <size_t N>
int execute_lines_or_err(Interp &interp, const char* (&lines)[N], const char *src_file, int line)
{
  for (auto l : lines) {
      auto res = (InterpReturn) print_interp_error(interp, interp.execute(l), src_file, line);
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
    settings->tool_table[0].toolno = 1 \

#endif // INTERP_TESTING_UTIL_HH
