#include "mock_xyza_config.hh"
#include "emc_nml.hh"
#include "emc_statics.hh"

void setup_xyza_config()
{
    *emcStatus = EMC_STAT();
    EMC_AXIS_STAT dummy_linear;
    dummy_linear.axisType = EMC_AXIS_LINEAR;
    dummy_linear.homed = true;
    dummy_linear.units = 1.0 / 25.4;
    dummy_linear.enabled = true;
    dummy_linear.maxPositionLimit = 20.0;
    dummy_linear.minPositionLimit = -10.0;
    emcStatus->motion.axis[X] = dummy_linear;
    emcStatus->motion.axis[Y] = dummy_linear;
    emcStatus->motion.axis[Z] = dummy_linear;
    emcStatus->motion.traj.axis_mask = 0xFF;
    emcStatus->motion.traj.axes = 9;
    emcStatus->motion.traj.linearUnits = 1.0 / 25.4;
    emcStatus->io.tool.toolTable[0].toolno = 1;
}
