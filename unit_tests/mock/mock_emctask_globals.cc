#include "emc_statics.hh"
#include "emc_nml.hh"

// global EMC status
int Argc;
char **Argv;

// full EMC status
EMC_STAT _emcStatus;
EMC_STAT *emcStatus = &_emcStatus;

// EMC IO status
EMC_IO_STAT _emcIoStatus;
EMC_IO_STAT *emcIoStatus = &_emcIoStatus;

// EMC MOTION status
EMC_MOTION_STAT *emcMotionStatus = nullptr;


void reset_mock_emcStatus()
{
    // TODO set up a default configuration here
}
