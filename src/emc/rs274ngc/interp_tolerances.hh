/********************************************************************
* Description: interp_tolerances.hh
*
*   Derived from a work by Thomas Kramer
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
********************************************************************/
#ifndef INTERP_TOLERANCES_HH
#define INTERP_TOLERANCES_HH

// English - Metric conversion (long number keeps error buildup down)
#define MM_PER_INCH 25.4

/* numerical constants */
//FIXME made very large here to allow badly formed arcs into canon
#define SPIRAL_TOLERANCE_INCH 1.0
// Note: started from original tolerance and divided by 10 here (since that was originally done inside the interpreter)
#define RADIUS_TOLERANCE_INCH 0.00005

/* Equivalent metric constants */
#define SPIRAL_TOLERANCE_MM (SPIRAL_TOLERANCE_INCH * MM_PER_INCH)
#define RADIUS_TOLERANCE_MM (RADIUS_TOLERANCE_INCH * MM_PER_INCH)

// Modest relative error
#define SPIRAL_RELATIVE_TOLERANCE 0.75

// If error values reach this fraction of the tolerance, a warning is thrown.
#define SPIRAL_WARNING_THRESHOLD 0.05

/* angle threshold for concavity for cutter compensation, in radians */
#define TOLERANCE_CONCAVE_CORNER 0.05  
#define TOLERANCE_EQUAL 0.0001 /* two numbers compare EQ if the
				  difference is less than this */

#define TINY 1e-12              /* for arc_data_r */

static inline bool equal(double a, double b)
{
    return (fabs(a - b) < TOLERANCE_EQUAL);
}

#endif

