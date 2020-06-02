#include "catch.hpp"
#include "canon.hh"
#include <sstream>
#include <cstdio>
#include <emc_statics.hh>
#include <emc_nml.hh>

#include <interpl.hh>

TEST_CASE("EMCCANON hello world")
{
    emcStatus->motion.traj.axis_mask = 0xF;
    INIT_CANON();
    while(interp_list.get()) {}
    SET_FEED_RATE(100.0);
    auto *tag_msg = interp_list.get();
    CHECK(tag_msg->type == EMC_TASK_SET_TAG_TYPE);
    ARC_FEED(0,1,2,3,4,5,6,7,8,9,1,2,3);
    auto *arc_msg = interp_list.get();
    CHECK(arc_msg->type == EMC_TRAJ_CIRCULAR_MOVE_TYPE);
}
