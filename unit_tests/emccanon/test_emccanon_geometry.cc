#include "catch.hpp"
#include "canon.hh"
#include <sstream>
#include <cstdio>
#include <emc_statics.hh>
#include <emc_nml.hh>

TEST_CASE("EMCCANON hello world")
{
    emcStatus->motion.traj.axis_mask = 0xF;
    INIT_CANON();
    SET_FEED_RATE(100.0);
    ARC_FEED(0,1,2,3,4,5,6,7,8,9,1,2,3);
}
