/********************************************************************
 * Description: canon_position.cc
 *
 *   Derived from a work by Thomas Kramer, implementing a simple 9D position
 *   structure.
 *
 * Author: Robert Ellenberg
 * License: GPL Version 2+
 * System: Linux
 *
 * Copyright (c) 2014 All rights reserved.
 *
 ********************************************************************/

#include "canon_position.hh"
#include "math.h"
#include "posemath.h"


PM_CARTESIAN xyz(PositionArray const &p) {
    return PM_CARTESIAN(p[X],
            p[Y],
            p[Z]);
}

void set_xyz(PositionArray &p, const PM_CARTESIAN & xyz)
{
    p[X] = xyz.x;
    p[Y] = xyz.y;
    p[Z] = xyz.z;
}

PM_CARTESIAN abc(PositionArray const &p) {
    return PM_CARTESIAN(p[A],
            p[B],
            p[C]);
}

void set_abc(PositionArray &p, const PM_CARTESIAN & abc)
{
    p[A] = abc.x;
    p[B] = abc.y;
    p[C] = abc.z;
}

PM_CARTESIAN uvw(PositionArray const &p) {
    return PM_CARTESIAN(p[U],
            p[V],
            p[W]);
}

void set_uvw(PositionArray &p, const PM_CARTESIAN & uvw)
{
    p[U] = uvw.x;
    p[V] = uvw.y;
    p[W] = uvw.z;
}
