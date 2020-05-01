#include "tp_debug.h"
#include "greatest.h"
#include "mock_standalone_tp.h"
#include "motion_struct.h"
#include "tp_priv.h"
#include "greatest_compare.h"
#include "posemath.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

#include "mock_rtapi.inc"

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    GREATEST_MAIN_END();        /* display results */
}
