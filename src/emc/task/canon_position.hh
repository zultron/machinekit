/********************************************************************
 * Description: canon_position.hh
 *
 *   CANON position class with operators and common functions
 *   Derived from a work by Thomas Kramer
 *
 * Author: Robert W. Ellenberg
 * License: GPL Version 2+
 * System: Linux
 *    
 * Copyright (c) 2014 All rights reserved.
 ********************************************************************/

#ifndef CANON_POSITION_HH
#define CANON_POSITION_HH

#include "posemath.h"       // For PM_CARTESIAN type
#include "positionarray.hh"

PM_CARTESIAN xyz(PositionArray const &p);
PM_CARTESIAN abc(PositionArray const &p);
PM_CARTESIAN uvw(PositionArray const &p);

void set_xyz(PositionArray &p, PM_CARTESIAN const &xyz);
void set_xyz(PositionArray &p, PM_CARTESIAN const &xyz);
void set_uvw(PositionArray &p, PM_CARTESIAN const &uvw);


#endif				/* ifndef CANON_POSITION_HH */
