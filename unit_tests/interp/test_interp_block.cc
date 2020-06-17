#include "catch.hpp"

using Catch::Matchers::Contains;

#include <interp_testing_util.hh> // For core interp stuff and extra REQUIRE macros/ setup
#include <rs274ngc_interp.hh>
#include <interp_inspection.hh>
#include <interp_return.hh>
#include <rs274ngc_return.hh>

#include <interp_parameter_def.hh>
using namespace interp_param_global;

/** Reads and parses a G-code command (and checks for errors along the way). */
static block_struct read_and_parse_command(Interp &test_interp, const char *command)
{
  char rawline[1024]="";
  char blocktext[1024]="";
  int line_length = 0;
  REQUIRE_INTERP_OK(test_interp, read_text(
                      command,
                      nullptr,
                      rawline,
                      blocktext,
                      &line_length));

  // Ensure it actually does something
  if (command && command[0]) {
    REQUIRE(line_length);
  }

  block_struct eblock{};
  REQUIRE_INTERP_OK(test_interp, init_block(&eblock));
  REQUIRE_INTERP_OK(test_interp, read_items(&eblock, blocktext, strlen(blocktext), test_interp._setup.parameters));
  return eblock;
}

SCENARIO("Movement commands")
{
  GIVEN("Linear Feed along multiple axes ")
  {
    DECL_INIT_TEST_INTERP_9AXIS();

    THEN("Block should have G1 word and axis words") {
      const char *command = "G1 X1 Y2 Z3 A4 C6";

      block_struct eblock = read_and_parse_command(test_interp, command);
      REQUIRE(eblock.g_modes[GM_MOTION] == G_1);
      REQUIRE(eblock.axis_flags[X]);
      REQUIRE(eblock.axis_flags[Y]);
      REQUIRE(eblock.axis_flags[Z]);
      REQUIRE(eblock.axis_flags[A]);
      REQUIRE(!eblock.axis_flags[B]);
      REQUIRE(eblock.axis_flags[C]);

      REQUIRE_FUZZ(eblock.axis_numbers[X], 1);
      REQUIRE_FUZZ(eblock.axis_numbers[Y], 2);
      REQUIRE_FUZZ(eblock.axis_numbers[Z], 3);
      REQUIRE_FUZZ(eblock.axis_numbers[A], 4);
      REQUIRE_FUZZ(eblock.axis_numbers[C], 6);
    }
  }
  GIVEN("Rapid along multiple axes ")
  {
    DECL_INIT_TEST_INTERP_9AXIS();

    THEN("Block should have G0 word and axis words") {
      const char *command = "G0 X1 Y2 Z3 A4 C6";

      block_struct eblock = read_and_parse_command(test_interp, command);
      REQUIRE(eblock.g_modes[GM_MOTION] == G_0);
      REQUIRE(eblock.axis_flags[X]);
      REQUIRE(eblock.axis_flags[Y]);
      REQUIRE(eblock.axis_flags[Z]);
      REQUIRE(eblock.axis_flags[A]);
      REQUIRE(!eblock.axis_flags[B]);
      REQUIRE(eblock.axis_flags[C]);

      REQUIRE_FUZZ(eblock.axis_numbers[X], 1);
      REQUIRE_FUZZ(eblock.axis_numbers[Y], 2);
      REQUIRE_FUZZ(eblock.axis_numbers[Z], 3);
      REQUIRE_FUZZ(eblock.axis_numbers[A], 4);
      REQUIRE_FUZZ(eblock.axis_numbers[C], 6);
    }
  }
  GIVEN("Clockwise circular motion commanded")
  {
    DECL_INIT_TEST_INTERP();
    const char *command = "G2 X1 Y2 Z3 R1";
    block_struct eblock = read_and_parse_command(test_interp, command);

    THEN("Block should have G2 / G3word and axis words") {

      REQUIRE(eblock.g_modes[GM_MOTION] == G_2);
      REQUIRE(eblock.axis_flags[X]);
      REQUIRE(eblock.axis_flags[Y]);
      REQUIRE(eblock.axis_flags[Z]);
      REQUIRE(eblock.r_flag);
      REQUIRE(!eblock.axis_flags[A]);
      REQUIRE(!eblock.axis_flags[B]);
      REQUIRE(!eblock.axis_flags[C]);

      REQUIRE_FUZZ(eblock.axis_numbers[X], 1);
      REQUIRE_FUZZ(eblock.axis_numbers[Y], 2);
      REQUIRE_FUZZ(eblock.axis_numbers[Z], 3);
      REQUIRE_FUZZ(eblock.r_number, 1);
    }
  }
}

SCENARIO("Conflicting meanings for G-code words")
{
    DECL_INIT_TEST_INTERP();
    GIVEN("A default interpreter")
    {
        REQUIRE_INTERP_OK(test_interp, execute("G90 G20"));
        REQUIRE_INTERP_OK(test_interp, execute("G0 X0 Y0 Z0"));
        REQUIRE_INTERP_OK(test_interp, execute("G91.1 F1"));
        THEN("G2 and G3 are not compatible with G4 due to P words") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G4 G2 X1 R1 F1 P1"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
            REQUIRE_INTERP_ERROR(test_interp, execute("G4 G3 X1 R1 F1 P1"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
        THEN("G64 and G2/G3 are not compatible") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G64 G2 X1 R1 F1 P1"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
            REQUIRE_INTERP_ERROR(test_interp, execute("G64 G3 X1 R1 F1 P1"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
        THEN("G4 and G64 are not compatible (P word)") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G4 G64 P2"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
        THEN("G10 and G64 are not compatible (P word)") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G10 L2 G64 P2 X2"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
        THEN("G64 and G82 are not compatible (P word)") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G64 G82 X2 Z-1 R1 P2"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
        THEN("G64 and G82 are not compatible (P word)") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G4 M100 P2 Q1 R4"));
            REQUIRE_THAT(test_interp.error_text(), Contains(NCE_P_WORD_CONFLICTING_MEANINGS));
        }
    }
}

SCENARIO("Multiple G-codes can't use the same axis words")
{
    DECL_INIT_TEST_INTERP();
    GIVEN("A default interpreter")
    {
        THEN("G10 and G82 are not compatible (P word)") {
            REQUIRE_INTERP_ERROR(test_interp, execute("G10 L2 G82 X2 Z-1 R1 P2"));
        }
    }
}

SCENARIO("Require that radius format arcs have at least one axis word from the current plane")
{
    DECL_INIT_TEST_INTERP();
        REQUIRE_INTERP_OK(test_interp, execute("G90 G21"));
    THEN("G17 requires X or Y") {
        REQUIRE_INTERP_OK(test_interp, execute("G0 X0 Y0 Z0"));
        REQUIRE_INTERP_OK(test_interp, execute("G1 X0 Y10 F1000"));
        REQUIRE_INTERP_OK(test_interp, execute("G17"));
        REQUIRE_INTERP_ERROR(test_interp, execute("G3 Z-10 R10"));
        REQUIRE_INTERP_OK(test_interp, execute("G3 Y20 R15"));
    }
    THEN("G18 requires X or Z") {
        REQUIRE_INTERP_OK(test_interp, execute("G0 X0 Y0 Z0"));
        REQUIRE_INTERP_OK(test_interp, execute("G1 X0 Y10 F1000"));
        REQUIRE_INTERP_OK(test_interp, execute("G18"));
        REQUIRE_INTERP_ERROR(test_interp, execute("G3 Y-10 R10"));
        REQUIRE_INTERP_OK(test_interp, execute("G3 Z20 R15"));
    }
    THEN("G19 requires Y or Z") {
        REQUIRE_INTERP_OK(test_interp, execute("G0 X0 Y0 Z0"));
        REQUIRE_INTERP_OK(test_interp, execute("G1 X0 Y10 F1000"));
        REQUIRE_INTERP_OK(test_interp, execute("G19"));
        REQUIRE_INTERP_ERROR(test_interp, execute("G3 X-10 R10"));
        REQUIRE_INTERP_OK(test_interp, execute("G3 Y20 R15"));
    }
}
