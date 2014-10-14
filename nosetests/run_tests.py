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
    # List of groups needing the realtime environment initialized
    # (almost everything)
    realtime_groups = [ "rtapi", "hal", "std_comps" ];

    # Groups in the realtime_groups list depend on "realtime_start"
    # group and will be run after the RT is started
    register(groups = realtime_groups,
             depends_on_groups = ["realtime_start"])

    # Groups depended upon by the "all_rt_tests" group will be run
    # before the RT environment is stopped
    register(groups = [ "all_rt_tests" ],
             depends_on_groups = realtime_groups)

    # Standard component tests should only be run if basic RTAPI and
    # HAL tests succeed
    register(groups = [ "std_comps" ],
             depends_on_groups = ["rtapi", "hal"])

    ############################################
    # Run proboscis
    #
    tp = TestProgram()

    #tp.show_plan()
    tp.run_and_exit()

if __name__ == '__main__':
    run_tests()
