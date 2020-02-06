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
#include "interp_parameter_def.hh"
#include "rotary_axis_util.hh"

using namespace interp_param_global;

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

DISTANCE_MODE Interp::get_distance_mode(block_pointer block,   //!< pointer to a block of RS274/NGC instructions
                                        setup_pointer settings //!< pointer to machine settings
                                        ) const
{
    // block non-modal distance mode overrides modal distance mode
    switch(block ? block->g_modes[GM_MODAL_0] : G_INVALID)
    {
    case G_91_2:
        return MODE_INCREMENTAL;
    case G_90_2:
        return MODE_ABSOLUTE;
    default:
        return settings->distance_mode;
    }
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
    
static bool angle_too_big(double absolute_ax_position) {
    bool outside_limits = absolute_ax_position < -360.0 || absolute_ax_position > 360.0;
    return !outside_limits;
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
                      double *u_p,
                      double *v_p,
                      double *w_p)
{
    PositionArray cmd_local;
    CHP(find_ends(block, s, cmd_local));
    extract_axes(cmd_local,
                 px,
                 py,
                 pz,
                 AA_p,
                 BB_p,
                 CC_p,
                 u_p,
                 v_p,
                 w_p);
    return INTERP_OK;
}

int Interp::find_ends(block_pointer block, setup_pointer s, PositionArray & cmd_local_pos)
{
    CHKS((block->radius_flag || block->theta_flag) && (block->x_flag || block->y_flag),
         _("Cannot specify X or Y words with polar coordinate"));

    if (block->g_modes[GM_MODAL_0] == G_53) {      /* distance mode is absolute in this case */
        const PositionArray machine_abs = s->transform.forward_local_to_tip(s->current);
        const PositionArray cmd_abs = getAxisWordsFromBlock(block, machine_abs);
        find_relative(s, cmd_abs, cmd_local_pos);
        return INTERP_OK;
    }

    PositionArray current = s->current;

    // If tool compensation is active, then the program_[xyz] fields have the actual current position that interp needs
    bool middle = !s->cutter_comp.firstmove;
    int comp = (s->cutter_comp.side);
    current[X] = (comp && middle) ? s->program_x : s->current[X];
    current[Y] = (comp && middle && s->plane == CANON_PLANE_XY) ? s->program_y : s->current[Y];
    current[Z] = (comp && middle && s->plane == CANON_PLANE_XZ) ? s->program_z : s->current[Z];

    auto distance_mode = get_distance_mode(block, s);
    switch(distance_mode) {
    case MODE_ABSOLUTE:
        cmd_local_pos = getAxisWordsFromBlock(block, current);

        if(block->radius_flag || block->theta_flag) {
            double radius = block->radius_flag ? block->radius : hypot(current[Y], current[X]);
            double theta = block->theta_flag ? D2R(block->theta) : atan2(current[Y], current[X]);
            cmd_local_pos[X] = radius * cos(theta);
            cmd_local_pos[Y] = radius * sin(theta);
        }

        if(block->a_flag && s->a_axis_wrapped) {
            CHP(unwrap_rotary(&cmd_local_pos[A], block->a_number, block->a_number, current[A], 'A'));
        }

        if(block->b_flag && s->b_axis_wrapped) {
            CHP(unwrap_rotary(&cmd_local_pos[B], block->b_number, block->b_number, current[B], 'B'));
        }

        if(block->c_flag && s->c_axis_wrapped) {
            CHP(unwrap_rotary(&cmd_local_pos[C], block->c_number, block->c_number, current[C], 'C'));
        }
        return INTERP_OK;
    case MODE_INCREMENTAL:
        cmd_local_pos = getAxisWordsFromBlock(block, PositionArray());
        cmd_local_pos += current;

        if(block->radius_flag || block->theta_flag) {
            CHKS((cmd_local_pos[Y] == 0 && cmd_local_pos[X] == 0), _("Incremental motion with polar coordinates is indeterminate when at the origin"));

            double radius_0 = hypot(cmd_local_pos[Y], cmd_local_pos[X]);
            double theta_0 = atan2(cmd_local_pos[Y], cmd_local_pos[X]);

            double radius = radius_0 + (block->radius_flag ? block->radius : 0);
            double theta = theta_0 + (block->theta_flag ? block->theta : 0);

            cmd_local_pos[X] = radius * cos(theta);
            cmd_local_pos[Y] = radius * sin(theta);
        }
        return INTERP_OK;
    }
    ERS(_("Invalid distance mode %d"), distance_mode);
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
    PositionArray cmd_abs;
    assign(cmd_abs,
            x1,
            y1,
            z1,
            AA_1,
            BB_1,
            CC_1,
            u_1,
            v_1,
            w_1);
    PositionArray cmd_local;
    find_relative(settings, cmd_abs, cmd_local);
    extract_axes(cmd_local,
            x2,
            y2,
            z2,
            AA_2,
            BB_2,
            CC_2,
            u_2,
            v_2,
            w_2);

  return INTERP_OK;
}

int Interp::find_relative(setup_pointer s, const PositionArray &cmd_abs, PositionArray &cmd_local)
{
    cmd_local = s->transform.inverse_local_from_tip(cmd_abs);
    CHKS(s->a_axis_wrapped && angle_too_big(cmd_abs[A]), (_("Invalid absolute position %5.2f for wrapped rotary axis %c")), cmd_abs[A], 'A');
    CHKS(s->b_axis_wrapped && angle_too_big(cmd_abs[B]), (_("Invalid absolute position %5.2f for wrapped rotary axis %c")), cmd_abs[B], 'B');
    CHKS(s->c_axis_wrapped && angle_too_big(cmd_abs[C]), (_("Invalid absolute position %5.2f for wrapped rotary axis %c")), cmd_abs[C], 'C');

    UnwrapsRotaryAxes unwrap(s->a_axis_wrapped, s->b_axis_wrapped, s->c_axis_wrapped);
    cmd_local = unwrap(cmd_abs, cmd_local, s->current);
    return INTERP_OK;
}

int Interp::clearPersistentG92Parameters(setup_pointer settings)
{
  static constexpr InterpParameterIndex to_clear[] = {
    G92_APPLIED,
    G92_X,
    G92_Y,
    G92_Z,
    G92_A,
    G92_B,
    G92_C,
    G92_U,
    G92_V,
    G92_W,
  };

  for (auto prm : to_clear) {
    settings->parameters[prm] = 0.0;
  }
  return INTERP_OK;
}

/**
 * Based on the current parameters, return the active offset (or zeros if it's inactive)
 * @warning only returns non-zero values if an offset is currently active in the parameter table.
 */
ShiftTransform Interp::getG92ParameterValues(setup_pointer settings)
{
    double *axis_offset_begin = &settings->parameters[G92_X];
    const UnitsTransform user_to_program = convertsUserToProgram();
    return ShiftTransform(user_to_program.forward(toArray(axis_offset_begin, 9)));
}

ShiftTransform Interp::getG92ActiveOffset(setup_pointer settings)
{
  if (settings->parameters[G92_APPLIED] != 0.0) {
    return getG92ParameterValues(settings);
  } else {
    return ShiftTransform();
  }
}

int Interp::storeG92OffsetValues(const ShiftTransform & g92_offset, setup_pointer settings)
{
  PositionArray g92_in_user_units = convertsProgramToUser()(g92_offset.tf);

  extract_axes(g92_in_user_units,
               settings->parameters[G92_X],
               settings->parameters[G92_Y],
               settings->parameters[G92_Z],
               settings->parameters[G92_A],
               settings->parameters[G92_B],
               settings->parameters[G92_C],
               settings->parameters[G92_U],
               settings->parameters[G92_V],
               settings->parameters[G92_W]
               );
  return INTERP_OK;
}

int Interp::storeG92Applied(bool active, setup_pointer settings)
{
    settings->parameters[G92_APPLIED] = active;
    return INTERP_OK;
}


int Interp::find_tool_offset(setup_pointer settings, int pocket, ShiftTransform & tool_offset)
{
  CHKS(pocket < 0 || pocket >= CANON_POCKETS_MAX, _("Invalid tool pocket number %d (must be between %d and %d"), pocket, 0, CANON_POCKETS_MAX);
  tool_offset = ShiftTransform(convertsUserToProgram()(toArray(settings->tool_table[pocket].offset)));
  return INTERP_OK;
}

int Interp::store_tool_offset(setup_pointer settings, const ShiftTransform & tool_offset, int pocket)
{
  CHKS(pocket < 0 || pocket >= CANON_POCKETS_MAX, _("Invalid tool pocket number %d (must be between %d and %d"), pocket, 0, CANON_POCKETS_MAX);
  settings->tool_table[pocket].offset = toEmcPose(convertsProgramToUser()(tool_offset.tf));
  settings->tool_offsets_changed.insert(settings->tool_table[pocket].toolno);
  return INTERP_OK;
}

int Interp::canon_use_tool_offset(const ShiftTransform & tool_offset)
{
    USE_TOOL_LENGTH_OFFSET(toEmcPose(tool_offset.tf));
    return INTERP_OK;
}

InterpReturn Interp::find_work_offset_from_parameters(setup_pointer settings, int origin, RotateShiftTransform &out)
{
    if (origin < 1 || origin >9) {
        return INTERP_ERROR;
    }
    const int prm_offset = getWorkOffsetParamIdx(origin, 1); // X starts at 1 in PRM table
    // KLUDGE assumes contiguous parameters X,Y,Z,...,R
    out = RotateShiftTransform(
        toArray(&settings->parameters.at(prm_offset),
                &settings->parameters[prm_offset + 9]),
        settings->parameters.at(prm_offset + 9));
    return INTERP_OK;
}

int Interp::store_work_offset_to_parameters(setup_pointer settings, const RotateShiftTransform & work_offset, int origin) const
{
    if (origin < 1 || origin >9) {
        return INTERP_ERROR;
    }
    const size_t prm_offset = (size_t)getWorkOffsetParamBase(origin);

    extract_axes(work_offset.shift.tf,
                 settings->parameters.at(1 + prm_offset),
                 settings->parameters[2 + prm_offset],
                 settings->parameters[3 + prm_offset],
                 settings->parameters[4 + prm_offset],
                 settings->parameters[5 + prm_offset],
                 settings->parameters[6 + prm_offset],
                 settings->parameters[7 + prm_offset],
                 settings->parameters[8 + prm_offset],
                 settings->parameters.at(9 + prm_offset));

    settings->parameters[10 + prm_offset] = work_offset.getXYRotationDeg(); // Rotation
    return INTERP_OK;
}

/** Finds the specified work offset in program units. */
int Interp::find_work_offset_prog(int origin, RotateShiftTransform &work_offset_prog)
{
  // Force an update from the parameters in case they changed underneath us during g-code read
  find_work_offset_from_parameters(&_setup, origin, _setup.work_offsets_user[origin]);
  work_offset_prog = convertsUserToProgram().forward_tf(_setup.work_offsets_user[origin]);
  return INTERP_OK;
}

int Interp::store_work_offset_from_prog(setup_pointer settings, const RotateShiftTransform & work_offset_prog, int origin) const
{
  return store_work_offset(settings, convertsProgramToUser().forward_tf(work_offset_prog), origin);
}

int Interp::store_work_offset(setup_pointer settings, RotateShiftTransform const &work_offset_user, int origin) const
{
    if (isNonzero(work_offset_user)) {
        settings->work_offsets_user[origin] = work_offset_user;
    } else {
        settings->work_offsets_user.erase(origin);
    }
    return store_work_offset_to_parameters(settings, work_offset_user, origin);
}

/** Find what current coordinates would be if the specified coordinate system was active. */
int Interp::find_current_in_system(setup_pointer s, int system, PositionArray &out) {
    // Tool offset is not changed by switching work offsets, so we don't have to transform fully into tool tip coordinates, just spindle nose coordinates
    PositionArray pos_nose = s->transform.forward_local_to_nose(s->current);
    LocalToMachineTransform tmp = s->transform;
    CHP(find_work_offset_prog(system, tmp.g5xOffset));

    out = tmp.inverse_local_from_nose(pos_nose);
    return INTERP_OK;
}

/**
 * Find what current coordinates would be if the specified coordinate system was active, and tool offsets were not applied.
 * Think of this as spindle-nose coordinates rather than tool-tip coordinates.
 */
int Interp::find_current_in_system_without_tlo(setup_pointer s, int system, PositionArray &out)
{
    PositionArray pos = s->transform.forward(s->current);
    LocalToMachineTransform tmp = s->transform;
    CHP(find_work_offset_prog(system, tmp.g5xOffset));
    // NOTE: this is where the "tool offsets not applied" part happens.
    out = tmp.inverse_local_from_nose(pos);
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
    if (hasFeature(FEATURE_LATHE_TOOLCHANGE)) {
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
