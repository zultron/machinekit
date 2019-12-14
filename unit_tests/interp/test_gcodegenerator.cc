#include "catch.hpp"
#include "gcodegenerator.hh"

using Catch::Matchers::Contains;

SCENARIO("Generate G-code from a modal state list")
{
    ModalState current{};

    current.g_codes[G_CODE_PLANE] = G_18;
    current.g_codes[G_CODE_SEQUENCE_NUMBER] = 55;
    current.g_codes[G_CODE_UNITS] = G_20;
    // KL
    current.g_codes[G_CODE_ORIGIN] = G_54;
    current.work_offset_number = 1;
    current.g_codes[G_CODE_DISTANCE_MODE] = G_90;
    current.g_codes[G_CODE_DISTANCE_MODE_IJK] = G_90_1;
    current.g_codes[G_CODE_MOTION_MODE] = G_1;

    ModalState saved = current;
    FeatureSet features(0x4+0x8);
    saved.g_codes[G_CODE_ORIGIN] = G_57;
    saved.work_offset_number = 3;
    saved.g_codes[G_CODE_DISTANCE_MODE] = G_91;
    saved.g_codes[G_CODE_DISTANCE_MODE_IJK] = G_91_1;
    saved.g_codes[G_CODE_MOTION_MODE] = G_3;

    GIVEN("Known modal state arrays")
    {
        THEN("G-code restore should produce equivalent G-code")
        {
            GCodeGenerator gen(features);
            gen.addModalStateGCodes(current, saved);
            std::string out = gen.getCommandSequenceAsString();

            CHECK_THAT(out, Contains("G54.1 P3", Catch::CaseSensitive::No));
            CHECK_THAT(out, Contains("G91", Catch::CaseSensitive::No));
            CHECK_THAT(out, Contains("G91.1", Catch::CaseSensitive::No));
            // Don't restore the motion mode
            CHECK_THAT(out, !Contains("G3", Catch::CaseSensitive::No));
            CHECK_THAT(out, !Contains("G41", Catch::CaseSensitive::No));
            CHECK_THAT(out, !Contains("G42", Catch::CaseSensitive::No));
            // TODO ensure that other G-codes don't sneak in here
        }
    }
}
