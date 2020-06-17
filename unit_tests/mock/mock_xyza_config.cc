#include "mock_xyza_config.hh"
#include "emc_nml.hh"
#include "emc_statics.hh"
#include "emc.hh"
#include "emcglb.h"

void setup_xyza_config()
{
    *emcStatus = EMC_STAT();
    {
        EMC_AXIS_STAT dummy_linear{};
        dummy_linear.axisType = EMC_AXIS_LINEAR;
        dummy_linear.homed = true;
        dummy_linear.units = 1.0 / 25.4;
        dummy_linear.enabled = true;
        dummy_linear.maxPositionLimit = 20.0;
        dummy_linear.minPositionLimit = -10.0;
        emcStatus->motion.axis[X] = dummy_linear;
        emcStatus->motion.axis[Y] = dummy_linear;
        emcStatus->motion.axis[Z] = dummy_linear;
    }
    {
        EMC_AXIS_STAT dummy_angular{};
        dummy_angular.axisType = EMC_AXIS_ANGULAR;
        dummy_angular.homed = true;
        dummy_angular.units = 1.0;
        dummy_angular.enabled = true;
        dummy_angular.maxPositionLimit = 1e99;
        dummy_angular.minPositionLimit = -1e99;
        emcStatus->motion.axis[A] = dummy_angular;
        emcStatus->motion.axis[B].enabled = false;
        emcStatus->motion.axis[C].enabled = false;
    }
    {
        emcStatus->motion.axis[U].enabled = false;
        emcStatus->motion.axis[V].enabled = false;
        emcStatus->motion.axis[W].enabled = false;
    }

    // TODO
    emcStatus->motion.traj.axis_mask = (1 << 4) - 1;
    emc_traj_axis_mask = emcStatus->motion.traj.axis_mask;
    emc_traj_axes = emcStatus->motion.traj.axes;
    emcStatus->motion.traj.axes = 4;
    emcStatus->motion.traj.linearUnits = 1.0 / 25.4;
    emcStatus->io.tool.toolTable[0].toolno = 1;
    emcStatus->io.tool.toolTable[1].toolno = 2;
    emcStatus->io.tool.toolTable[2].toolno = 3;
}

void setup_9axis_config()
{
    *emcStatus = EMC_STAT();
    {
        EMC_AXIS_STAT dummy_linear{};
        dummy_linear.axisType = EMC_AXIS_LINEAR;
        dummy_linear.homed = true;
        dummy_linear.units = 1.0 / 25.4;
        dummy_linear.enabled = true;
        dummy_linear.maxPositionLimit = 20.0;
        dummy_linear.minPositionLimit = -10.0;
        emcStatus->motion.axis[X] = dummy_linear;
        emcStatus->motion.axis[Y] = dummy_linear;
        emcStatus->motion.axis[Z] = dummy_linear;
        emcStatus->motion.axis[U] = dummy_linear;
        emcStatus->motion.axis[V] = dummy_linear;
        emcStatus->motion.axis[W] = dummy_linear;
    }
    {
        EMC_AXIS_STAT dummy_angular{};
        dummy_angular.axisType = EMC_AXIS_ANGULAR;
        dummy_angular.homed = true;
        dummy_angular.units = 1.0;
        dummy_angular.enabled = true;
        dummy_angular.maxPositionLimit = 1e99;
        dummy_angular.minPositionLimit = -1e99;
        emcStatus->motion.axis[A] = dummy_angular;
        emcStatus->motion.axis[B] = dummy_angular;
        emcStatus->motion.axis[C] = dummy_angular;
    }

    emcStatus->motion.traj.axis_mask = (1 << 9) - 1;
    emc_traj_axis_mask = emcStatus->motion.traj.axis_mask;
    emc_traj_axes = emcStatus->motion.traj.axes;
    emcStatus->motion.traj.axes = 9;
    emcStatus->motion.traj.linearUnits = 1.0 / 25.4;
    emcStatus->io.tool.toolTable[0].toolno = 1;
    emcStatus->io.tool.toolTable[1].toolno = 2;
    emcStatus->io.tool.toolTable[2].toolno = 3;

    // KLUDGE force emcglobals to have reasonable values for axis kinematic limits
    // TODO set these in a more standard way for tests that require specific values
    axis_max_velocity[X] = 2.0;
    axis_max_velocity[Y] = 2.0;
    axis_max_velocity[Z] = 1.5;
    axis_max_acceleration[X] = 15.0;
    axis_max_acceleration[Y] = 15.0;
    axis_max_acceleration[Z] = 10.0;
}
