/********************************************************************
* Description: interp_find.cc
*
*   Derived from a work by Thomas Kramer
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rs274ngc.hh"
#include "interp_return.hh"
#include "interp_internal.hh"
#include "rs274ngc_interp.hh"
#include "units.h"

/****************************************************************************/

/*! find_arc_length

Returned Value: double (length of path between start and end points)

Side effects: none

Called by:
   inverse_time_rate_arc
   inverse_time_rate_arc2
   inverse_time_rate_as

This calculates the length of the path that will be made relative to
the XYZ axes for a motion in which the X,Y,Z, motion is a circular or
helical arc with its axis parallel to the Z-axis. If tool length
compensation is on, this is the path of the tool tip; if off, the
length of the path of the spindle tip. Any rotary axis motion is
ignored.

If the arc is helical, it is coincident with the hypotenuse of a right
triangle wrapped around a cylinder. If the triangle is unwrapped, its
base is [the radius of the cylinder times the number of radians in the
helix] and its height is [z2 - z1], and the path length can be found
by the Pythagorean theorem.

This is written as though it is only for arcs whose axis is parallel to
the Z-axis, but it will serve also for arcs whose axis is parallel
to the X-axis or Y-axis, with suitable permutation of the arguments.

This works correctly when turn is zero (find_turn returns 0 in that
case).

*/

double Interp::find_arc_length(double x1,        //!< X-coordinate of start point       
                              double y1,        //!< Y-coordinate of start point       
                              double z1,        //!< Z-coordinate of start point       
                              double center_x,  //!< X-coordinate of arc center        
                              double center_y,  //!< Y-coordinate of arc center        
                              int turn, //!< no. of full or partial circles CCW
                              double x2,        //!< X-coordinate of end point         
                              double y2,        //!< Y-coordinate of end point         
                              double z2)        //!< Z-coordinate of end point         
{
  double radius;
  double theta;                 /* amount of turn of arc in radians */

  radius = hypot((center_x - x1), (center_y - y1));
  theta = find_turn(x1, y1, center_x, center_y, turn, x2, y2);
  if (z2 == z1)
    return (radius * fabs(theta));
  else
    return hypot((radius * theta), (z2 - z1));
}


/* Find the real destination, given the axis's current position, the
   commanded destination, and the direction to turn (which comes from
   the sign of the commanded value in the gcode).  Modulo 360 positions
   of the axis are considered equivalent and we just need to find the
   nearest one. */

int Interp::unwrap_rotary(double *r, double sign_of, double commanded, double current, char axis) {
    double result;
    int neg = copysign(1.0, sign_of) < 0.0;
    CHKS((sign_of <= -360.0 || sign_of >= 360.0), (_("Invalid absolute position %5.2f for wrapped rotary axis %c")), sign_of, axis);
    
    double d = floor(current/360.0);
    result = fabs(commanded) + (d*360.0);
    if(!neg && result < current) result += 360.0;
    if(neg && result > current) result -= 360.0;
    *r = result;

    return INTERP_OK;
}
    

/****************************************************************************/

/*! find_ends

Returned Value: int (INTERP_OK)

Side effects:
   The values of px, py, pz, aa_p, bb_p, and cc_p are set

Called by:
   convert_arc
   convert_home
   convert_probe
   convert_straight

This finds the coordinates of a point, "end", in the currently
active coordinate system, and sets the values of the pointers to the
coordinates (which are the arguments to the function).

In all cases, if no value for the coodinate is given in the block, the
current value for the coordinate is used. When cutter radius
compensation is on, this function is called before compensation
calculations are performed, so the current value of the programmed
point is used, not the current value of the actual current_point.

There are three cases for when the coordinate is included in the block:

1. G_53 is active. This means to interpret the coordinates as machine
coordinates. That is accomplished by adding the two offsets to the
coordinate given in the block.

2. Absolute coordinate mode is in effect. The coordinate in the block
is used.

3. Incremental coordinate mode is in effect. The coordinate in the
block plus either (i) the programmed current position - when cutter
radius compensation is in progress, or (2) the actual current position.

*/


int Interp::find_ends(block_pointer block,       //!< pointer to a block of RS274/NGC instructions
                      setup_pointer s,    //!< pointer to machine settings                 
                      double *px,        //!< pointer to end_x                            
                      double *py,        //!< pointer to end_y                            
                      double *pz,        //!< pointer to end_z                            
                      double *AA_p,      //!< pointer to end_a                      
                      double *BB_p,      //!< pointer to end_b                      
                      double *CC_p,      //!< pointer to end_c                      
                      double *u_p, double *v_p, double *w_p)
{
    int middle;
    int comp;

    middle = !s->cutter_comp_firstmove;
    comp = (s->cutter_comp_side);

    if (block->g_modes[0] == G_53) {      /* distance mode is absolute in this case */
#ifdef DEBUG_EMC
        COMMENT("interpreter: offsets temporarily suspended");
#endif
        CHKS((block->radius_flag || block->theta_flag), _("Cannot use polar coordinates with G53"));

        double cx = s->current[X];
        double cy = s->current[Y];
        rotate(&cx, &cy, s->transform.g5xOffset.getXYRotationDeg()); //FIXME transform

        if(block->x_flag) {
            *px = block->x_number - s->transform.g5xOffset.shift.tf[X] - s->transform.g92Offset.tf[X] - s->tool_offset.tran.x;
        } else {
            *px = cx;
        }

        if(block->y_flag) {
            *py = block->y_number - s->transform.g5xOffset.shift.tf[Y] - s->transform.g92Offset.tf[Y] - s->tool_offset.tran.y;
        } else {
            *py = cy;
        }

        rotate(px, py, -s->transform.g5xOffset.getXYRotationDeg()); // FIXME transform

        if(block->z_flag) {
            *pz = block->z_number - s->transform.g5xOffset.shift.tf[Z] - s->transform.g92Offset.tf[Z] - s->tool_offset.tran.z;
        } else {
            *pz = s->current[Z];
        }

        if(block->a_flag) {
            if(s->a_axis_wrapped) {
                CHP(unwrap_rotary(AA_p, block->a_number, 
                                  block->a_number - s->transform.g5xOffset.shift.tf[A] - s->transform.g92Offset.tf[A] - s->tool_offset.a,
                                  s->current[A], 'A'));
            } else {
                *AA_p = block->a_number - s->transform.g5xOffset.shift.tf[A] - s->transform.g92Offset.tf[A];
            }
        } else {
            *AA_p = s->current[A];
        }

        if(block->b_flag) {
            if(s->b_axis_wrapped) {
                CHP(unwrap_rotary(BB_p, block->b_number, 
                                  block->b_number - s->transform.g5xOffset.shift.tf[B] - s->transform.g92Offset.tf[B] - s->tool_offset.b,
                                  s->current[B], 'B'));
            } else {
                *BB_p = block->b_number - s->transform.g5xOffset.shift.tf[B] - s->transform.g92Offset.tf[B];
            }
        } else {
            *BB_p = s->current[B];
        }

        if(block->c_flag) {
            if(s->c_axis_wrapped) {
                CHP(unwrap_rotary(CC_p, block->c_number, 
                                  block->c_number - s->transform.g5xOffset.shift.tf[C] - s->transform.g92Offset.tf[C] - s->tool_offset.c,
                                  s->current[C], 'C'));
            } else {
                *CC_p = block->c_number - s->transform.g5xOffset.shift.tf[C] - s->transform.g92Offset.tf[C];
            }
        } else {
            *CC_p = s->current[C];
        }

        if(block->u_flag) {
            *u_p = block->u_number - s->transform.g5xOffset.shift.tf[U] - s->transform.g92Offset.tf[U] - s->tool_offset.u;
        } else {
            *u_p = s->current[U];
        }

        if(block->v_flag) {
            *v_p = block->v_number - s->transform.g5xOffset.shift.tf[V] - s->transform.g92Offset.tf[V] - s->tool_offset.v;
        } else {
            *v_p = s->current[V];
        }

        if(block->w_flag) {
            *w_p = block->w_number - s->transform.g5xOffset.shift.tf[W] - s->transform.g92Offset.tf[W] - s->tool_offset.w;
        } else {
            *w_p = s->current[W];
        }
    } else if (s->distance_mode == MODE_ABSOLUTE) {

        if(block->x_flag) {
            *px = block->x_number;
        } else {
            // both cutter comp planes affect X ...
            *px = (comp && middle) ? s->program_x : s->current[X];
        }

        if(block->y_flag) {
            *py = block->y_number;
        } else {
            // but only XY affects Y ...
            *py = (comp && middle && s->plane == CANON_PLANE_XY) ? s->program_y : s->current[Y];
        }

        if(block->radius_flag && block->theta_flag) {
            CHKS((block->x_flag || block->y_flag), _("Cannot specify X or Y words with polar coordinate"));
            *px = block->radius * cos(D2R(block->theta));
            *py = block->radius * sin(D2R(block->theta));
        } else if(block->radius_flag) {
            double theta;
            CHKS((block->x_flag || block->y_flag), _("Cannot specify X or Y words with polar coordinate"));
            CHKS((*py == 0 && *px == 0), _("Must specify angle in polar coordinate if at the origin"));
            theta = atan2(*py, *px);
            *px = block->radius * cos(theta);
            *py = block->radius * sin(theta);
        } else  if(block->theta_flag) {
            double radius;
            CHKS((block->x_flag || block->y_flag), _("Cannot specify X or Y words with polar coordinate"));
            radius = hypot(*py, *px);
            *px = radius * cos(D2R(block->theta));
            *py = radius * sin(D2R(block->theta));
        }

        if(block->z_flag) {
            *pz = block->z_number;
        } else {
            // and only XZ affects Z.
            *pz = (comp && middle && s->plane == CANON_PLANE_XZ) ? s->program_z : s->current[Z];
        }

        if(block->a_flag) {
            if(s->a_axis_wrapped) {
                CHP(unwrap_rotary(AA_p, block->a_number, block->a_number, s->current[A], 'A'));
            } else {
                *AA_p = block->a_number;
            }
        } else {
            *AA_p = s->current[A];
        }

        if(block->b_flag) {
            if(s->b_axis_wrapped) {
                CHP(unwrap_rotary(BB_p, block->b_number, block->b_number, s->current[B], 'B'));
            } else {
                *BB_p = block->b_number;
            }
        } else {
            *BB_p = s->current[B];
        }

        if(block->c_flag) {
            if(s->c_axis_wrapped) {
                CHP(unwrap_rotary(CC_p, block->c_number, block->c_number, s->current[C], 'C'));
            } else {
                *CC_p = block->c_number;
            }
        } else {
            *CC_p = s->current[C];
        }

        *u_p = (block->u_flag) ? block->u_number : s->current[U];
        *v_p = (block->v_flag) ? block->v_number : s->current[V];
        *w_p = (block->w_flag) ? block->w_number : s->current[W];

    } else {                      /* mode is MODE_INCREMENTAL */

        // both cutter comp planes affect X ...
        *px = (comp && middle) ? s->program_x: s->current[X];
        if(block->x_flag) *px += block->x_number;

        // but only XY affects Y ...
        *py = (comp && middle && s->plane == CANON_PLANE_XY) ? s->program_y: s->current[Y];
        if(block->y_flag) *py += block->y_number;

        if(block->radius_flag) {
            double radius, theta;
            CHKS((block->x_flag || block->y_flag), _("Cannot specify X or Y words with polar coordinate"));
            CHKS((*py == 0 && *px == 0), _("Incremental motion with polar coordinates is indeterminate when at the origin"));
            theta = atan2(*py, *px);
            radius = hypot(*py, *px) + block->radius;
            *px = radius * cos(theta);
            *py = radius * sin(theta);
        }

        if(block->theta_flag) {
            double radius, theta;
            CHKS((block->x_flag || block->y_flag), _("Cannot specify X or Y words with polar coordinate"));
            CHKS((*py == 0 && *px == 0), _("G91 motion with polar coordinates is indeterminate when at the origin"));
            theta = atan2(*py, *px) + D2R(block->theta);
            radius = hypot(*py, *px);
            *px = radius * cos(theta);
            *py = radius * sin(theta);
        }

        // and only XZ affects Z.
        *pz = (comp && middle && s->plane == CANON_PLANE_XZ) ? s->program_z: s->current[Z];
        if(block->z_flag) *pz += block->z_number;

        *AA_p = s->current[A];
        if(block->a_flag) *AA_p += block->a_number;

        *BB_p = s->current[B];
        if(block->b_flag) *BB_p += block->b_number;

        *CC_p = s->current[C];
        if(block->c_flag) *CC_p += block->c_number;

        *u_p = s->current[U];
        if(block->u_flag) *u_p += block->u_number;

        *v_p = s->current[V];
        if(block->v_flag) *v_p += block->v_number;

        *w_p = s->current[W];
        if(block->w_flag) *w_p += block->w_number;
    }
    return INTERP_OK;
}

/****************************************************************************/

/*! find_relative

Returned Value: int (INTERP_OK)

Side effects:
   The values of x2, y2, z2, aa_2, bb_2, and cc_2 are set.
   (NOTE: aa_2 etc. are written with lower case letters in this
    documentation because upper case would confuse the pre-preprocessor.)

Called by:
   convert_home

This finds the coordinates in the current system, under the current
tool length offset, of a point (x1, y1, z1, aa_1, bb_1, cc_1) whose absolute
coordinates are known.

Don't confuse this with the inverse operation.

*/

int Interp::find_relative(double x1,     //!< absolute x position        
                          double y1,     //!< absolute y position        
                          double z1,     //!< absolute z position        
                          double AA_1,   //!< absolute a position         
                          double BB_1,   //!< absolute b position         
                          double CC_1,   //!< absolute c position         
                          double u_1,
                          double v_1,
                          double w_1,
                          double *x2,    //!< pointer to relative x      
                          double *y2,    //!< pointer to relative y      
                          double *z2,    //!< pointer to relative z      
                          double *AA_2,  //!< pointer to relative a       
                          double *BB_2,  //!< pointer to relative b       
                          double *CC_2,  //!< pointer to relative c       
                          double *u_2,
                          double *v_2,
                          double *w_2,
                          setup_pointer settings)        //!< pointer to machine settings
{
  *x2 = x1 - settings->transform.g5xOffset.shift.tf[X] - settings->transform.g92Offset.tf[X] - settings->tool_offset.tran.x;
  *y2 = y1 - settings->transform.g5xOffset.shift.tf[Y] - settings->transform.g92Offset.tf[Y] - settings->tool_offset.tran.y;
  rotate(x2, y2, -settings->transform.g5xOffset.getXYRotationDeg()); // FIXME transform
  *z2 = z1 - settings->transform.g5xOffset.shift.tf[Z] - settings->transform.g92Offset.tf[Z] - settings->tool_offset.tran.z;

  if(settings->a_axis_wrapped) {
      CHP(unwrap_rotary(AA_2, AA_1,
                        AA_1 - settings->transform.g5xOffset.shift.tf[A] - settings->transform.g92Offset.tf[A] - settings->tool_offset.a,
                        settings->current[A], 'A'));
  } else {
      *AA_2 = AA_1 - settings->transform.g5xOffset.shift.tf[A] - settings->transform.g92Offset.tf[A];
  }

  if(settings->b_axis_wrapped) {
      CHP(unwrap_rotary(BB_2, BB_1,
                        BB_1 - settings->transform.g5xOffset.shift.tf[B] - settings->transform.g92Offset.tf[B] - settings->tool_offset.b,
                        settings->current[B], 'B'));
  } else {
      *BB_2 = BB_1 - settings->transform.g5xOffset.shift.tf[B] - settings->transform.g92Offset.tf[B];
  }

  if(settings->c_axis_wrapped) {
      CHP(unwrap_rotary(CC_2, CC_1,
                        CC_1 - settings->transform.g5xOffset.shift.tf[C] - settings->transform.g92Offset.tf[C] - settings->tool_offset.c,
                        settings->current[C], 'C'));
  } else {
      *CC_2 = CC_1 - settings->transform.g5xOffset.shift.tf[C] - settings->transform.g92Offset.tf[C];
  }

  *u_2 = u_1 - settings->transform.g5xOffset.shift.tf[U] - settings->transform.g92Offset.tf[U] - settings->tool_offset.u;
  *v_2 = v_1 - settings->transform.g5xOffset.shift.tf[V] - settings->transform.g92Offset.tf[V] - settings->tool_offset.v;
  *w_2 = w_1 - settings->transform.g5xOffset.shift.tf[W] - settings->transform.g92Offset.tf[W] - settings->tool_offset.w;
  return INTERP_OK;
}

// find what the current coordinates would be if we were in a different system

int Interp::find_current_in_system(setup_pointer s, int system,
                                   double *x, double *y, double *z,
                                   double *a, double *b, double *c,
                                   double *u, double *v, double *w) {
    double *p = s->parameters;

    *x = s->current[X];
    *y = s->current[Y];
    *z = s->current[Z];
    *a = s->current[A];
    *b = s->current[B];
    *c = s->current[C];
    *u = s->current[U];
    *v = s->current[V];
    *w = s->current[W];

    *x += s->transform.g92Offset.tf[X];
    *y += s->transform.g92Offset.tf[Y];
    *z += s->transform.g92Offset.tf[Z];
    *a += s->transform.g92Offset.tf[A];
    *b += s->transform.g92Offset.tf[B];
    *c += s->transform.g92Offset.tf[C];
    *u += s->transform.g92Offset.tf[U];
    *v += s->transform.g92Offset.tf[V];
    *w += s->transform.g92Offset.tf[W];

    rotate(x, y, s->transform.g5xOffset.getXYRotationDeg()); //FIXME transform

    *x += s->transform.g5xOffset.shift.tf[X];
    *y += s->transform.g5xOffset.shift.tf[Y];
    *z += s->transform.g5xOffset.shift.tf[Z];
    *a += s->transform.g5xOffset.shift.tf[A];
    *b += s->transform.g5xOffset.shift.tf[B];
    *c += s->transform.g5xOffset.shift.tf[C];
    *u += s->transform.g5xOffset.shift.tf[U];
    *v += s->transform.g5xOffset.shift.tf[V];
    *w += s->transform.g5xOffset.shift.tf[W];

    *x -= USER_TO_PROGRAM_LEN(p[5201 + system * 20]);
    *y -= USER_TO_PROGRAM_LEN(p[5202 + system * 20]);
    *z -= USER_TO_PROGRAM_LEN(p[5203 + system * 20]);
    *a -= USER_TO_PROGRAM_ANG(p[5204 + system * 20]);
    *b -= USER_TO_PROGRAM_ANG(p[5205 + system * 20]);
    *c -= USER_TO_PROGRAM_ANG(p[5206 + system * 20]);
    *u -= USER_TO_PROGRAM_LEN(p[5207 + system * 20]);
    *v -= USER_TO_PROGRAM_LEN(p[5208 + system * 20]);
    *w -= USER_TO_PROGRAM_LEN(p[5209 + system * 20]);

    rotate(x, y, -p[5210 + system * 20]);

    if (p[5210]) {
        *x -= USER_TO_PROGRAM_LEN(p[5211]);
        *y -= USER_TO_PROGRAM_LEN(p[5212]);
        *z -= USER_TO_PROGRAM_LEN(p[5213]);
        *a -= USER_TO_PROGRAM_ANG(p[5214]);
        *b -= USER_TO_PROGRAM_ANG(p[5215]);
        *c -= USER_TO_PROGRAM_ANG(p[5216]);
        *u -= USER_TO_PROGRAM_LEN(p[5217]);
        *v -= USER_TO_PROGRAM_LEN(p[5218]);
        *w -= USER_TO_PROGRAM_LEN(p[5219]);
    }

    return INTERP_OK;
}


// find what the current coordinates would be if we were in a different system,
// if TLO were unapplied

int Interp::find_current_in_system_without_tlo(setup_pointer s, int system,
                                   double *x, double *y, double *z,
                                   double *a, double *b, double *c,
                                   double *u, double *v, double *w) {
    double *p = s->parameters;

    *x = s->current[X];
    *y = s->current[Y];
    *z = s->current[Z];
    *a = s->current[A];
    *b = s->current[B];
    *c = s->current[C];
    *u = s->current[U];
    *v = s->current[V];
    *w = s->current[W];

    *x += s->transform.g92Offset.tf[X];
    *y += s->transform.g92Offset.tf[Y];
    *z += s->transform.g92Offset.tf[Z];
    *a += s->transform.g92Offset.tf[A];
    *b += s->transform.g92Offset.tf[B];
    *c += s->transform.g92Offset.tf[C];
    *u += s->transform.g92Offset.tf[U];
    *v += s->transform.g92Offset.tf[V];
    *w += s->transform.g92Offset.tf[W];

    rotate(x, y, s->transform.g5xOffset.getXYRotationDeg()); // FIXME transform

    *x += s->transform.g5xOffset.shift.tf[X];
    *y += s->transform.g5xOffset.shift.tf[Y];
    *z += s->transform.g5xOffset.shift.tf[Z];
    *a += s->transform.g5xOffset.shift.tf[A];
    *b += s->transform.g5xOffset.shift.tf[B];
    *c += s->transform.g5xOffset.shift.tf[C];
    *u += s->transform.g5xOffset.shift.tf[U];
    *v += s->transform.g5xOffset.shift.tf[V];
    *w += s->transform.g5xOffset.shift.tf[W];

    *x += s->tool_offset.tran.x;
    *y += s->tool_offset.tran.y;
    *z += s->tool_offset.tran.z;
    *a += s->tool_offset.a;
    *b += s->tool_offset.b;
    *c += s->tool_offset.c;
    *u += s->tool_offset.u;
    *v += s->tool_offset.v;
    *w += s->tool_offset.w;

    *x -= USER_TO_PROGRAM_LEN(p[5201 + system * 20]);
    *y -= USER_TO_PROGRAM_LEN(p[5202 + system * 20]);
    *z -= USER_TO_PROGRAM_LEN(p[5203 + system * 20]);
    *a -= USER_TO_PROGRAM_ANG(p[5204 + system * 20]);
    *b -= USER_TO_PROGRAM_ANG(p[5205 + system * 20]);
    *c -= USER_TO_PROGRAM_ANG(p[5206 + system * 20]);
    *u -= USER_TO_PROGRAM_LEN(p[5207 + system * 20]);
    *v -= USER_TO_PROGRAM_LEN(p[5208 + system * 20]);
    *w -= USER_TO_PROGRAM_LEN(p[5209 + system * 20]);

    rotate(x, y, -p[5210 + system * 20]);

    if (p[5210]) {
        *x -= USER_TO_PROGRAM_LEN(p[5211]);
        *y -= USER_TO_PROGRAM_LEN(p[5212]);
        *z -= USER_TO_PROGRAM_LEN(p[5213]);
        *a -= USER_TO_PROGRAM_ANG(p[5214]);
        *b -= USER_TO_PROGRAM_ANG(p[5215]);
        *c -= USER_TO_PROGRAM_ANG(p[5216]);
        *u -= USER_TO_PROGRAM_LEN(p[5217]);
        *v -= USER_TO_PROGRAM_LEN(p[5218]);
        *w -= USER_TO_PROGRAM_LEN(p[5219]);
    }

    return INTERP_OK;
}

/****************************************************************************/

/*! find_straight_length

Returned Value: double (length of path between start and end points)

Side effects: none

Called by:
  inverse_time_rate_straight
  inverse_time_rate_as

This calculates a number to use in feed rate calculations when inverse
time feed mode is used, for a motion in which X,Y,Z,A,B, and C each change
linearly or not at all from their initial value to their end value.

This is used when the feed_reference mode is CANON_XYZ, which is
always in rs274NGC.

If any of the X, Y, or Z axes move or the A-axis, B-axis, and C-axis
do not move, this is the length of the path relative to the XYZ axes
from the first point to the second, and any rotary axis motion is
ignored. The length is the simple Euclidean distance.

The formula for the Euclidean distance "length" of a move involving
only the A, B and C axes is based on a conversation with Jim Frohardt at
Boeing, who says that the Fanuc controller on their 5-axis machine
interprets the feed rate this way. Note that if only one rotary axis
moves, this formula returns the absolute value of that axis move,
which is what is desired.

*/

double Interp::find_straight_length(double x2,   //!< X-coordinate of end point   
                                    double y2,   //!< Y-coordinate of end point   
                                    double z2,   //!< Z-coordinate of end point   
                                    double AA_2, //!< A-coordinate of end point    
                                    double BB_2, //!< B-coordinate of end point    
                                    double CC_2, //!< C-coordinate of end point    
                                    double u_2,
                                    double v_2,
                                    double w_2,
                                    double x1,   //!< X-coordinate of start point 
                                    double y1,   //!< Y-coordinate of start point 
                                    double z1,    //!< Z-coordinate of start point 
                                    double AA_1,        //!< A-coordinate of start point  
                                    double BB_1,        //!< B-coordinate of start point  
                                    double CC_1,        //!< C-coordinate of start point  
                                    double u_1,
                                    double v_1,
                                    double w_1
  )
{
#define tiny 1e-7
    if ( (fabs(x1-x2) > tiny) || (fabs(y1-y2) > tiny) || (fabs(z1-z2) > tiny) )
        return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2) + pow((z2 - z1), 2));
    else if ( (fabs(u_1-u_2) > tiny) || (fabs(v_1-v_2) > tiny) || (fabs(w_1-w_2) > tiny) )
        return sqrt(pow((u_2 - u_1), 2) + pow((v_2 - v_1), 2) + pow((w_2 - w_1), 2));
    else
        return sqrt(pow((AA_2 - AA_1), 2) + pow((BB_2 - BB_1), 2) + pow((CC_2 - CC_1), 2));
}

/****************************************************************************/

/*! find_turn

Returned Value: double (angle in radians between two radii of a circle)

Side effects: none

Called by: find_arc_length

All angles are in radians.

*/

double Interp::find_turn(double x1,      //!< X-coordinate of start point       
                        double y1,      //!< Y-coordinate of start point       
                        double center_x,        //!< X-coordinate of arc center        
                        double center_y,        //!< Y-coordinate of arc center        
                        int turn,       //!< no. of full or partial circles CCW
                        double x2,      //!< X-coordinate of end point         
                        double y2)      //!< Y-coordinate of end point         
{
  double alpha;                 /* angle of first radius */
  double beta;                  /* angle of second radius */
  double theta;                 /* amount of turn of arc CCW - negative if CW */

  if (turn == 0)
    return 0.0;
  alpha = atan2((y1 - center_y), (x1 - center_x));
  beta = atan2((y2 - center_y), (x2 - center_x));
  if (turn > 0) {
    if (beta <= alpha)
      beta = (beta + (2 * M_PIl));
    theta = ((beta - alpha) + ((turn - 1) * (2 * M_PIl)));
  } else {                      /* turn < 0 */

    if (alpha <= beta)
      alpha = (alpha + (2 * M_PIl));
    theta = ((beta - alpha) + ((turn + 1) * (2 * M_PIl)));
  }
  return (theta);
}

int Interp::find_tool_pocket(setup_pointer settings, int toolno, int *pocket)
{
    if (FEATURE(LATHE_TOOLCHANGE)) {
		if (toolno > 99 && toolno < 10000) { //Future change required for using tools greater than 99
			toolno = floor(toolno / 100);
		}
	}
    if(!settings->random_toolchanger && toolno == 0) {
        *pocket = 0;
        return INTERP_OK;
    }
    *pocket = -1;
    for(int i=0; i<CANON_POCKETS_MAX; i++) {
        if(settings->tool_table[i].toolno == toolno)
            *pocket = i;
    }

    CHKS((*pocket == -1), (_("Requested tool %d not found in the tool table")), toolno);
    return INTERP_OK;
}
