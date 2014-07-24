/********************************************************************
* Description: emc.hh
*   Declarations for EMC NML vocabulary
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/
#ifndef EMC_STATICS_HH
#define EMC_STATICS_HH

#include "config.h"
#include "emcglb.h"		// EMC_AXIS_MAX
#include "nml_type.hh"
#include "motion_types.h"
#include "emc_types.hh"

// --------------
// EMC VOCABULARY
// --------------

// decls for command line args-- mains are responsible for setting these
// so that other modules can get cmd line args for ad hoc processing
extern int Argc;
extern char **Argv;

// full EMC status
extern EMC_STAT *emcStatus;

// EMC IO status
extern EMC_IO_STAT *emcIoStatus;

// EMC MOTION status
extern EMC_MOTION_STAT *emcMotionStatus;

#endif				// #ifndef EMC_STATICS_HH
