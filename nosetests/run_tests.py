from proboscis import TestProgram, register
from proboscis.decorators import DEFAULT_REGISTRY
import utils

def run_tests():
    ############################################
    # Test module imports
    #
    # Add new modules here
    #
    # rtapi group
    import test_rtapi
    import test_compat
    # hal group
    import test_mk_hal_basics
    import test_pinops
    import test_netcmd
    import test_groups
    import test_ring
    import test_ring_intracomp
    import test_streamring
    import test_ringdemo
    # std_comps group
    import test_or2
    

    ############################################
    # Group dependencies
    #
    # realtime_start
    # realtime
    # rtapi_base
    # rtapi
    # hal_pins
    # hal_ring
    # hal_base
    # hal
    # comp_env

    # "realtime" group: Anything depending on this has the base RT
    # environment established, with no functionality tested.

    # "rtapi" group: This group tests the basic RTAPI environment.

    # "hal" group: This group tests the basic HAL environment.

    # "base" group: Anything depending on this will already have a
    # tested RT and HAL environment established.
    register(groups = ["base"],
             depends_on_groups = ["rtapi", "hal"])

    # "std_comp" group: Test standard components.  Tests in this group
    # should depend on "base".

    # Groups depended upon by the "all_rt_tests" group will be run
    # before the RT environment is stopped
    register(groups = [ "all_rt_tests" ],
             depends_on_groups = ["base", "std_comp"])

    ############################################
    # Run proboscis
    #
    tp = TestProgram()

    #tp.show_plan()
    tp.run_and_exit()

if __name__ == '__main__':
    run_tests()
