/********************************************************************
* Description: interp_cycles.cc
*
*   The bulk of the functions here control how canned cycles are
*   interpreted.
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
********************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rs274ngc.hh"
#include "rs274ngc_return.hh"
#include "interp_internal.hh"
#include "rs274ngc_interp.hh"
#include "posemath_ext.hh"

static const char* plane_name(CANON_PLANE p);

/****************************************************************************/

/*! convert_cycle_g81

Returned Value: int (INTERP_OK)

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle, which
is usually drilling:
1. Move the z-axis only at the current feed rate to the specified bottom_z.
2. Retract the z-axis at traverse rate to clear_z.

See [NCMS, page 99].

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g81(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z)   //!< value of z at bottom of cycle   
{
    cycle_feed(cycle_end,plane, x, y, bottom_z);
    cycle_traverse(cycle_end, plane, x, y, clear_z);

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g82

Returned Value: int (INTERP_OK)

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle, which
is usually drilling:
1. Move the z_axis only at the current feed rate to the specified z-value.
2. Dwell for the given number of seconds.
3. Retract the z-axis at traverse rate to the clear_z.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g82(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z,   //!< value of z at bottom of cycle   
                              double dwell)      //!< dwell time                      
{
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  DWELL(dwell);
  cycle_traverse(cycle_end, plane, x, y, clear_z);

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g83

Returned Value: int (INTERP_OK)

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle,
which is usually peck drilling:
1. Move the z-axis only at the current feed rate downward by delta or
   to the specified bottom_z, whichever is less deep.
2. Rapid back out to the R plane.
3. Rapid back down to the current hole bottom, backed off a bit.
4. Repeat steps 1, 2, and 3 until the specified bottom_z is reached.
5. Retract the z-axis at traverse rate to clear_z.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

The rapid out and back in causes any long stringers (which are common
when drilling in aluminum) to be cut off and clears chips from the
hole.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g83(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double r,  //!< initial z-value                 
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z,   //!< value of z at bottom of cycle   
                              double delta)      //!< size of z-axis feed increment   
{
  double current_depth;
  double rapid_delta;

  /* Moved the check for negative Q values here as a sign
     may be used with user defined M functions
     Thanks to Billy Singleton for pointing it out... */
  CHKS((delta <= 0.0), NCE_NEGATIVE_OR_ZERO_Q_VALUE_USED);

  rapid_delta = G83_RAPID_DELTA;
  if (_setup.length_units == CANON_UNITS_MM)
    rapid_delta = (rapid_delta * 25.4);

  for (current_depth = (r - delta);
       current_depth > bottom_z; current_depth = (current_depth - delta)) {
    cycle_feed(cycle_end,plane, x, y, current_depth);
    cycle_traverse(cycle_end, plane, x, y, r);
    cycle_traverse(cycle_end, plane, x, y, current_depth + rapid_delta);
  }
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  cycle_traverse(cycle_end, plane, x, y, clear_z);

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g73

Returned Value: int (INTERP_OK)

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle,
which is usually peck drilling:
1. Move the z-axis only at the current feed rate downward by delta or
   to the specified bottom_z, whichever is less deep.
2. Rapid back out a bit.
4. Repeat steps 1, 2, and 3 until the specified bottom_z is reached.
5. Retract the z-axis at traverse rate to clear_z.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

The rapid out and back in causes any long stringers (which are common
when drilling in aluminum) to be cut off and clears chips from the
hole.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g73(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double r,  //!< initial z-value                 
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z,   //!< value of z at bottom of cycle   
                              double delta)      //!< size of z-axis feed increment   
{
  double current_depth;
  double rapid_delta;

  /* Moved the check for negative Q values here as a sign
     may be used with user defined M functions
     Thanks to Billy Singleton for pointing it out... */
  CHKS((delta <= 0.0), NCE_NEGATIVE_OR_ZERO_Q_VALUE_USED);

  rapid_delta = G83_RAPID_DELTA;
  if (_setup.length_units == CANON_UNITS_MM)
    rapid_delta = (rapid_delta * 25.4);

  for (current_depth = (r - delta);
       current_depth > bottom_z; current_depth = (current_depth - delta)) {
    cycle_feed(cycle_end,plane, x, y, current_depth);
    cycle_traverse(cycle_end, plane, x, y, current_depth + rapid_delta);
  }
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  cycle_traverse(cycle_end, plane, x, y, clear_z);

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g74_g84

Returned Value: int
   If the spindle is not turning clockwise and motion is not g84, this returns
      NCE_SPINDLE_NOT_TURNING_CLOCKWISE_IN_G84.
   If the spindle is not turning counterclockwise and motion is not g74, this returns
      NCE_SPINDLE_NOT_TURNING_COUNTER_CLOCKWISE_IN_G74.

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle,
+which is right-hand floating-chuck tapping:
1. start the feed move towards bottom_z.
2. Move the z-axis only at the current feed rate to the specified bottom_z.
3. Stop the spindle.
4. Start the spindle counterclockwise (or cw if g74), suppressing the
   wait-for-spindle-at-speed for the following feed move out.
5. Dwell for the specified time (optional)
6. Retract the z-axis at current feed rate to clear_z.
7. Stop the spindle.
8. Start the spindle clockwise (or ccw if g74), with normal wait-for-at-speed for next feed move.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.
The direction argument must be clockwise.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g74_g84(PositionArray const &cycle_end,
                                 CANON_PLANE plane, //!< selected plane
                                 double planar_pos_1,  //!< "x"-value where cycle is executed
                                 double planar_pos_2,  //!< "y"-value where cycle is executed
                                 double clear_height,    //!< height of clearance plane
                                 double bottom_depth,   //!< depth at bottom of cycle
                                 CANON_DIRECTION direction, //!< direction spindle turning at outset
                                 CANON_SPEED_FEED_MODE mode,       //!< the speed-feed mode at outset
                                 int motion,
                                  double dwell)
{
    char op = motion ==  G_74_1 ? '7' : '8';

    switch (direction) {
    case CANON_STOPPED:
        ERS(_("Spindle not turning in G%c4"), op);
    case CANON_CLOCKWISE:
        CHKS((motion == G_74_1), _("Spindle turning clockwise in G%c4"), op);
        break;
    case CANON_COUNTERCLOCKWISE:
        CHKS((motion == G_84_1), _("Spindle turning counterclockwise in G%c4"), op);
    }

    switch (plane) {
    case CANON_PLANE_UV:
    case CANON_PLANE_VW:
    case CANON_PLANE_UW:
        ERS(_("Tap cycles G74 / G84 not supported in UVW axes"));
    default:
        break;
    }

    int save_feed_override_enable;
    int save_spindle_override_enable;

    save_feed_override_enable = GET_EXTERNAL_FEED_OVERRIDE_ENABLE();
    save_spindle_override_enable = GET_EXTERNAL_SPINDLE_OVERRIDE_ENABLE();

    DISABLE_FEED_OVERRIDE();
    DISABLE_SPEED_OVERRIDE();
    if (motion == G_84_1)
        START_SPINDLE_CLOCKWISE();
    else
        START_SPINDLE_COUNTERCLOCKWISE();
    cycle_feed(cycle_end,plane, planar_pos_1, planar_pos_2, bottom_depth);
    STOP_SPINDLE_TURNING();
    // the zero parameter suppresses the wait for at-speed on next feed
    if (motion == G_84_1)
        START_SPINDLE_COUNTERCLOCKWISE(0);
    else
        START_SPINDLE_CLOCKWISE(0);
    // NOTE: use internal TP dwell for more precise delay time
    if (dwell > 0) {
        PRECISION_DWELL(dwell);
    } else {
        // NOTE: fast-retract not currently supported for non-rigid tapping
        // FIXME assumes
        double delta_rpm = GET_EXTERNAL_SPEED_PLANNED() * 2.0;
        DWELL_FOR_SPINDLE(delta_rpm);
    }
    cycle_feed(cycle_end,plane, planar_pos_1, planar_pos_2, clear_height);
    STOP_SPINDLE_TURNING();
    if (motion == G_84_1)
        START_SPINDLE_CLOCKWISE();
    else
        START_SPINDLE_COUNTERCLOCKWISE();

    if(save_feed_override_enable)
        ENABLE_FEED_OVERRIDE();
    if(save_spindle_override_enable)
        ENABLE_SPEED_OVERRIDE();

    return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g85

Returned Value: int (INTERP_OK)

Side effects:
   A number of moves are made as described below.

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle,
which is usually boring or reaming:
1. Move the z-axis only at the current feed rate to the specified z-value.
2. Retract the z-axis at the current feed rate to clear_z.

CYCLE_MACRO has positioned the tool at (x, y, r, ?, ?) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g85(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double r,  // retract plane
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z)   //!< value of z at bottom of cycle   
{
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  cycle_feed(cycle_end,plane, x, y, r);
  cycle_traverse(cycle_end, plane, x, y, clear_z);

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g86

Returned Value: int
   If the spindle is not turning clockwise or counterclockwise,
   this returns NCE_SPINDLE_NOT_TURNING_IN_G86.
   Otherwise, it returns INTERP_OK.

Side effects:
   A number of moves are made as described below.

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the RS274/NGC following cycle,
which is usually boring:
1. Move the z-axis only at the current feed rate to bottom_z.
2. Dwell for the given number of seconds.
3. Stop the spindle turning.
4. Retract the z-axis at traverse rate to clear_z.
5. Restart the spindle in the direction it was going.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g86(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                     
                              double x,  //!< x-value where cycle is executed    
                              double y,  //!< y-value where cycle is executed    
                              double clear_z,    //!< z-value of clearance plane         
                              double bottom_z,   //!< value of z at bottom of cycle      
                              double dwell,      //!< dwell time                         
                              CANON_DIRECTION direction) //!< direction spindle turning at outset
{
  CHKS(((direction != CANON_CLOCKWISE) &&
       (direction != CANON_COUNTERCLOCKWISE)),
      NCE_SPINDLE_NOT_TURNING_IN_G86);

  cycle_feed(cycle_end,plane, x, y, bottom_z);
  DWELL(dwell);
  STOP_SPINDLE_TURNING();
  DWELL_FOR_SPINDLE(GET_EXTERNAL_SPEED_PLANNED(), 2.0); // KLUDGE simulate at-speed but for zero speed
  cycle_traverse(cycle_end, plane, x, y, clear_z);
  if (direction == CANON_CLOCKWISE)
    START_SPINDLE_CLOCKWISE();
  else
    START_SPINDLE_COUNTERCLOCKWISE();

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g87

Returned Value: int
   If the spindle is not turning clockwise or counterclockwise,
   this returns NCE_SPINDLE_NOT_TURNING_IN_G87.
   Otherwise, it returns INTERP_OK.

Side effects:
   A number of moves are made as described below. This cycle is a
   modified version of [Monarch, page 5-24] since [NCMS, pages 98 - 100]
   gives no clue as to what the cycle is supposed to do. [KT] does not
   have a back boring cycle. [Fanuc, page 132] in "Canned cycle II"
   describes the G87 cycle as given here, except that the direction of
   spindle turning is always clockwise and step 7 below is omitted
   in [Fanuc].

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle, which
is usually back boring.  The situation is that you have a through hole
and you want to counterbore the bottom of hole. To do this you put an
L-shaped tool in the spindle with a cutting surface on the UPPER side
of its base. You stick it carefully through the hole when it is not
spinning and is oriented so it fits through the hole, then you move it
so the stem of the L is on the axis of the hole, start the spindle,
and feed the tool upward to make the counterbore. Then you get the
tool out of the hole.

1. Move at traverse rate parallel to the XY-plane to the point
   with x-value offset_x and y-value offset_y.
2. Stop the spindle in a specific orientation.
3. Move the z-axis only at traverse rate downward to the bottom_z.
4. Move at traverse rate parallel to the XY-plane to the x,y location.
5. Start the spindle in the direction it was going before.
6. Move the z-axis only at the given feed rate upward to the middle_z.
7. Move the z-axis only at the given feed rate back down to bottom_z.
8. Stop the spindle in the same orientation as before.
9. Move at traverse rate parallel to the XY-plane to the point
   with x-value offset_x and y-value offset_y.
10. Move the z-axis only at traverse rate to the clear z value.
11. Move at traverse rate parallel to the XY-plane to the specified x,y
    location.
12. Restart the spindle in the direction it was going before.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) before this starts.

It might be useful to add a check that clear_z > middle_z > bottom_z.
Without the check, however, this can be used to counterbore a hole in
material that can only be accessed through a hole in material above it.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g87(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                     
                              double x,  //!< x-value where cycle is executed    
                              double offset_x,   //!< x-axis offset position             
                              double y,  //!< y-value where cycle is executed    
                              double offset_y,   //!< y-axis offset position             
                              double r,  //!< z_value of r_plane                 
                              double clear_z,    //!< z-value of clearance plane         
                              double middle_z,   //!< z-value of top of back bore        
                              double bottom_z,   //!< value of z at bottom of cycle      
                              CANON_DIRECTION direction) //!< direction spindle turning at outset
{
  CHKS(((direction != CANON_CLOCKWISE) &&
       (direction != CANON_COUNTERCLOCKWISE)),
      NCE_SPINDLE_NOT_TURNING_IN_G87);

  cycle_traverse(cycle_end, plane, offset_x, offset_y, r);
  STOP_SPINDLE_TURNING();
  ORIENT_SPINDLE(0.0, direction);
  cycle_traverse(cycle_end, plane, offset_x, offset_y, bottom_z);
  cycle_traverse(cycle_end, plane, x, y, bottom_z);
  if (direction == CANON_CLOCKWISE)
    START_SPINDLE_CLOCKWISE();
  else
    START_SPINDLE_COUNTERCLOCKWISE();
  cycle_feed(cycle_end,plane, x, y, middle_z);
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  STOP_SPINDLE_TURNING();
  ORIENT_SPINDLE(0.0, direction);
  cycle_traverse(cycle_end, plane, offset_x, offset_y, bottom_z);
  cycle_traverse(cycle_end, plane, offset_x, offset_y, clear_z);
  cycle_traverse(cycle_end, plane, x, y, clear_z);
  if (direction == CANON_CLOCKWISE)
    START_SPINDLE_CLOCKWISE();
  else
    START_SPINDLE_COUNTERCLOCKWISE();

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g88

Returned Value: int
   If the spindle is not turning clockwise or counterclockwise, this
   returns NCE_SPINDLE_NOT_TURNING_IN_G88.
   Otherwise, it returns INTERP_OK.

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

For the XY plane, this implements the following RS274/NGC cycle,
which is usually boring:
1. Move the z-axis only at the current feed rate to the specified z-value.
2. Dwell for the given number of seconds.
3. Stop the spindle turning.
4. Stop the program so the operator can retract the spindle manually.
5. Restart the spindle.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g88(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                     
                              double x,  //!< x-value where cycle is executed    
                              double y,  //!< y-value where cycle is executed    
                              double bottom_z,   //!< value of z at bottom of cycle      
                              double dwell,      //!< dwell time                         
                              CANON_DIRECTION direction) //!< direction spindle turning at outset
{
  CHKS(((direction != CANON_CLOCKWISE) &&
       (direction != CANON_COUNTERCLOCKWISE)),
      NCE_SPINDLE_NOT_TURNING_IN_G88);

  cycle_feed(cycle_end,plane, x, y, bottom_z);
  DWELL(dwell);
  STOP_SPINDLE_TURNING();
  /* operator retracts the spindle here */
  // FIXME write state tag?
  //write_and_update_state_tag(block);
  PROGRAM_STOP();
  if (direction == CANON_CLOCKWISE)
    START_SPINDLE_CLOCKWISE();
  else
    START_SPINDLE_COUNTERCLOCKWISE();

  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_g89

Returned Value: int (INTERP_OK)

Side effects: See below

Called by:
   convert_cycle_xy
   convert_cycle_yz
   convert_cycle_zx

This implements the following RS274/NGC cycle, which is intended for boring:
1. Move the z-axis only at the current feed rate to the specified z-value.
2. Dwell for the given number of seconds.
3. Retract the z-axis at the current feed rate to clear_z.

CYCLE_MACRO has positioned the tool at (x, y, r, a, b, c) when this starts.

For the XZ and YZ planes, this makes analogous motions.

*/

int Interp::convert_cycle_g89(PositionArray const &cycle_end,
                              CANON_PLANE plane, //!< selected plane                  
                              double x,  //!< x-value where cycle is executed 
                              double y,  //!< y-value where cycle is executed 
                              double clear_z,    //!< z-value of clearance plane      
                              double bottom_z,   //!< value of z at bottom of cycle   
                              double dwell)      //!< dwell time                      
{
  cycle_feed(cycle_end,plane, x, y, bottom_z);
  DWELL(dwell);
  cycle_feed(cycle_end,plane, x, y, clear_z);

  return INTERP_OK;
}

static const char* plane_name(CANON_PLANE p) {
  switch(p) {
    case CANON_PLANE_XY: return "XY";
    case CANON_PLANE_YZ: return "YZ";
    case CANON_PLANE_XZ: return "XZ";
    case CANON_PLANE_UV: return "UV";
    case CANON_PLANE_VW: return "VW";
    case CANON_PLANE_UW: return "UW";
  }
  return "invalid";
}

/****************************************************************************/

/*! convert_cycle

Returned Value: int
   If any of the specific functions called returns an error code,
   this returns that code.
   If any of the following errors occur, this returns the error code shown.
   Otherwise, it returns INTERP_OK.
   1. The r-value is not given the first time this code is called after
      some other motion mode has been in effect:
      NCE_R_CLEARANCE_PLANE_UNSPECIFIED_IN_CYCLE
   2. The l number is zero: NCE_CANNOT_DO_ZERO_REPEATS_OF_CYCLE
   3. The currently selected plane in not XY, YZ, or XZ.
      NCE_BUG_PLANE_NOT_XY_YZ_OR_XZ

Side effects:
   A number of moves are made to execute a canned cycle. The current
   position is reset. The values of the cycle attributes in the settings
   may be reset.

Called by: convert_motion

This function makes a couple checks and then calls one of three
functions, according to which plane is currently selected.

See the documentation of convert_cycle_xy for most of the details.

*/

int Interp::convert_cycle(GCodes motion,    //!< a g-code between G_81 and G_89, a canned cycle
                         block_pointer block,   //!< pointer to a block of RS274 instructions      
                         setup_pointer settings)        //!< pointer to machine settings                   
{
  CHKS((settings->feed_rate == 0.0), _("Cannot feed with zero feed rate"));
  CHKS((settings->feed_mode == INVERSE_TIME), _("Cannot use inverse time feed with canned cycles"));
  CHKS((settings->cutter_comp.side), _("Cannot use canned cycles with cutter compensation on"));

  CANON_PLANE plane = settings->plane;
  if (!block->r_flag) {
    if (settings->motion_mode == motion)
      block->r_number = settings->cycle.r;
    else
      ERS(NCE_R_CLEARANCE_PLANE_UNSPECIFIED_IN_CYCLE);
  }

  CHKS((block->l_number == 0), NCE_CANNOT_DO_ZERO_REPEATS_OF_CYCLE);
  if (block->l_number == -1)
    block->l_number = 1;

  switch(plane) {
  case CANON_PLANE_XY:
  case CANON_PLANE_XZ:
  case CANON_PLANE_YZ:
    CHKS(block->axis_flags[U], "Cannot put a U in a canned cycle in the %s plane",
	plane_name(settings->plane));
    CHKS(block->axis_flags[V], "Cannot put a V in a canned cycle in the %s plane",
	plane_name(settings->plane));
    CHKS(block->axis_flags[W], "Cannot put a W in a canned cycle in the %s plane",
	plane_name(settings->plane));
    break;

  case CANON_PLANE_UV:
  case CANON_PLANE_VW:
  case CANON_PLANE_UW:
    CHKS(block->axis_flags[X], "Cannot put an X in a canned cycle in the %s plane",
	plane_name(settings->plane));
    CHKS(block->axis_flags[Y], "Cannot put a Y in a canned cycle in the %s plane",
	plane_name(settings->plane));
    CHKS(block->axis_flags[Z], "Cannot put a Z in a canned cycle in the %s plane",
	plane_name(settings->plane));
  }

    //KLUDGE ugly way to save / restore motion mode flag so that state tag displays correctly
    GCodes save_mode = settings->motion_mode;
    settings->motion_mode = motion;
    write_and_update_state_tag(block);
    settings->motion_mode = save_mode;
    // end KLUDGE

  CHP(convert_cycle_for_plane(motion, block, settings));

  settings->cycle.l = block->l_number;
  settings->cycle.r = block->r_number;
  settings->motion_mode = motion;
  return INTERP_OK;
}

/****************************************************************************/

/*! convert_cycle_for_plane

Returned Value: int
   If any of the specific functions called returns an error code,
   this returns that code.
   If any of the following errors occur, this returns the error code shown.
   Otherwise, it returns INTERP_OK.
   1. The z-value is not given the first time this code is called after
      some other motion mode has been in effect:
      NCE_Z_VALUE_UNSPECIFIED_IN_XY_PLANE_CANNED_CYCLE
   2. The r clearance plane is below the bottom_z:
      NCE_R_LESS_THAN_Z_IN_CYCLE_IN_XY_PLANE
   3. the distance mode is neither absolute or incremental:
      NCE_BUG_DISTANCE_MODE_NOT_G90_OR_G91
   4. G82, G86, G88, or G89 is called when it is not already in effect,
      and no p number is in the block:
      NCE_DWELL_TIME_P_WORD_MISSING_WITH_G82
      NCE_DWELL_TIME_P_WORD_MISSING_WITH_G86
      NCE_DWELL_TIME_P_WORD_MISSING_WITH_G88
      NCE_DWELL_TIME_P_WORD_MISSING_WITH_G89
   5. G83 is called when it is not already in effect,
      and no q number is in the block: NCE_Q_WORD_MISSING_WITH_G83_OR_M66
   6. G87 is called when it is not already in effect,
      and any of the i number, j number, or k number is missing:
      NCE_I_WORD_MISSING_WITH_G87
      NCE_J_WORD_MISSING_WITH_G87
      NCE_K_WORD_MISSING_WITH_G87
   7. the G code is not between G_81 and G_89.
      NCE_BUG_FUNCTION_SHOULD_NOT_HAVE_BEEN_CALLED

Side effects:
   A number of moves are made to execute the g-code

Called by: convert_cycle

The function does not require that any of x,y,z, or r be specified in
the block, except that if the last motion mode command executed was
not the same as this one, the r-value and z-value must be specified.

This function is handling the repeat feature of RS274/NGC, wherein
the L word represents the number of repeats [NCMS, page 99]. We are
not allowing L=0, contrary to the manual. We are allowing L > 1
in absolute distance mode to mean "do the same thing in the same
place several times", as provided in the manual, although this seems
abnormal.

In incremental distance mode, x, y, and r values are treated as
increments to the current position and z as an increment from r.  In
absolute distance mode, x, y, r, and z are absolute. In g87, i and j
will always be increments, regardless of the distance mode setting, as
implied in [NCMS, page 98], but k (z-value of top of counterbore) will
be an absolute z-value in absolute distance mode, and an increment
(from bottom z) in incremental distance mode.

If the r position of a cycle is above the current_z position, this
retracts the z-axis to the r position before moving parallel to the
XY plane.

In the code for this function, there is a nearly identical "for" loop
in every case of the switch. The loop is the done with a compiler
macro, "CYCLE_MACRO" so that the code is easy to read, automatically
kept identical from case to case and, and much shorter than it would
be without the macro. The loop could be put outside the switch, but
then the switch would run every time around the loop, not just once,
as it does here. The loop could also be placed in the called
functions, but then it would not be clear that all the loops are the
same, and it would be hard to keep them the same when the code is
modified.  The macro would be very awkward as a regular function
because it would have to be passed all of the arguments used by any of
the specific cycles, and, if another switch in the function is to be
avoided, it would have to passed a function pointer, but the different
cycle functions have different arguments so the type of the pointer
could not be declared unless the cycle functions were re-written to
take the same arguments (in which case most of them would have several
unused arguments).

The motions within the CYCLE_MACRO (but outside a specific cycle) are
a straight traverse parallel to the selected plane to the given
position in the plane and a straight traverse of the third axis only
(if needed) to the r position.

The CYCLE_MACRO is defined here but is also used in convert_cycle_yz
and convert_cycle_zx. The variables aa, bb, and cc are used in
CYCLE_MACRO and in the other two functions just mentioned. Those
variables represent the first axis of the selected plane, the second
axis of the selected plane, and third axis which is perpendicular to
the selected plane.  In this function aa represents x, bb represents
y, and cc represents z. This usage makes it possible to have only one
version of each of the cycle functions.  The cycle_traverse and
cycle_feed functions help accomplish this.

The height of the retract move at the end of each repeat of a cycle is
determined by the setting of the retract_mode: either to the r
position (if the retract_mode is R_PLANE) or to the original
z-position (if that is above the r position and the retract_mode is
not R_PLANE). This is a slight departure from [NCMS, page 98], which
does not require checking that the original z-position is above r.

*/
int Interp::convert_cycle_for_plane(int motion, //!< a g-code between G_81 and G_89, a canned cycle
                            block_pointer block,        //!< pointer to a block of RS274 instructions
                            setup_pointer settings)     //!< pointer to machine settings
{
    const CANON_PLANE plane = settings->plane;
    PositionArrayIndex axis_0, axis_1, depth_axis; std::tie(axis_0, axis_1, depth_axis) = plane_indices(plane);

    double current_cc = settings->current[depth_axis];

    double aa;
    double aa_increment=0.;
    double bb;
    double bb_increment=0.;
    double cc;
    double clear_cc;
    double i;
    double j;
    double k;
    double old_cc;
    double radius_increment = 0.;
    double theta_increment = 0.;
    double r;
    int repeat;
    CANON_MOTION_MODE save_mode;
    double save_tolerance;
    PositionArray cycle_end{};
    PositionArray cycle_increment{};

    if (settings->motion_mode != motion) {
        char depth_axis_name = axis_to_char(depth_axis);
        CHKS(!_readers[(int)std::tolower(depth_axis_name)], _("Canned cycle is not possible in %c%c plane on a machine without a %c axis"),
             axis_to_char(axis_0),
             axis_to_char(axis_1),
             depth_axis_name);

        CHKS((!block->axis_flags[depth_axis]), _("%c word required for canned cycle in %c%c plane"),
             depth_axis_name,
             axis_to_char(axis_0),
             axis_to_char(axis_1));
    }

    block->axis_numbers[depth_axis] =
        block->axis_flags[depth_axis] ? block->axis_numbers[depth_axis] : settings->cycle.cc;

    if(settings->cycle.il_flag) {
        old_cc = settings->cycle.il;
    } else {
        old_cc = settings->cycle.il = current_cc;
        settings->cycle.il_flag = true;
    }

    if (get_distance_mode(block, settings) == MODE_ABSOLUTE) {
        cc = block->axis_numbers[depth_axis];
        r = block->r_number;
        if(block->radius_flag || block->theta_flag) {
            get_absolute_polar(block, settings->current[axis_0], settings->current[axis_1], aa, bb);
        } else {
            aa = block->axis_flags[axis_0] ? block->axis_numbers[axis_0] : settings->current[axis_0];
            bb = block->axis_flags[axis_1] ? block->axis_numbers[axis_1] : settings->current[axis_1];
        }
        cycle_end = getAxisWordsFromBlock(block, _setup.current);

    } else if (get_distance_mode(block, settings) == MODE_INCREMENTAL) {
        if (block->radius_flag) radius_increment = block->radius;
        if (block->theta_flag) theta_increment = D2R(block->theta);
        if (block->axis_flags[axis_0]) aa_increment = block->axis_numbers[axis_0];
        if (block->axis_flags[axis_1]) bb_increment = block->axis_numbers[axis_1];

        // Allow polar displacement after linear
        if(block->radius_flag || block->theta_flag) {
            add_relative_polar(block, aa, bb);
        }

        aa = settings->current[axis_0];
        bb = settings->current[axis_1];
        r = (block->r_number + old_cc);
        cc = (r + block->axis_numbers[depth_axis]); /* [NCMS, page 98] */
        cycle_end = _setup.current;
        cycle_increment = getAxisWordsFromBlock(block, PositionArray());
    } else
        ERS(NCE_BUG_DISTANCE_MODE_NOT_G90_OR_G91);
    CHKS((r < cc), NCE_R_LESS_THAN_Z_IN_CYCLE_IN_XY_PLANE);

    // First motion of a canned cycle (maybe): if we're below the R plane,
    // rapid straight up to the R plane.
    if (old_cc < r) {
        PositionArray rapid_up_to_r = settings->current;
        rapid_up_to_r[depth_axis] = r;
        STRAIGHT_TRAVERSE(block->line_number, UNPACK_POSITION_ARRAY_REF(rapid_up_to_r));
        old_cc = r;
        current_cc = old_cc;
    }
    clear_cc = (settings->cycle.retract_mode == R_PLANE) ? r : old_cc;

    save_mode = GET_EXTERNAL_MOTION_CONTROL_MODE();
    save_tolerance = GET_EXTERNAL_MOTION_CONTROL_TOLERANCE();
    if (save_mode != CANON_EXACT_PATH)
        SET_MOTION_CONTROL_MODE(CANON_EXACT_PATH, 0);

    switch (motion) {
    case G_81:
        CYCLE_MACRO(convert_cycle_g81(cycle_end, plane, aa, bb, clear_cc, cc))
        break;
    case G_82:
        CHKS(((settings->motion_mode != G_82) && (block->p_number == -1.0)),
             NCE_DWELL_TIME_P_WORD_MISSING_WITH_G82);
        block->p_number =
            block->p_number == -1.0 ? settings->cycle.p : block->p_number;
        CYCLE_MACRO(convert_cycle_g82(cycle_end, plane, aa, bb, clear_cc, cc,
                                      block->p_number))
        settings->cycle.p = block->p_number;
        break;
    case G_73:
        CHKS(((settings->motion_mode != G_73) && (block->q_number == -1.0)),
             NCE_Q_WORD_MISSING_WITH_G73);
        block->q_number =
            block->q_number == -1.0 ? settings->cycle.q : block->q_number;
        CYCLE_MACRO(convert_cycle_g73(cycle_end, plane, aa, bb, r, clear_cc, cc,
                                      block->q_number))
        settings->cycle.q = block->q_number;
        break;
    case G_83:
        CHKS(((settings->motion_mode != G_83) && (block->q_number == -1.0)),
             NCE_Q_WORD_MISSING_WITH_G83);
        block->q_number =
            block->q_number == -1.0 ? settings->cycle.q : block->q_number;
        CYCLE_MACRO(convert_cycle_g83(cycle_end, plane, aa, bb, r, clear_cc, cc,
                                      block->q_number))
        settings->cycle.q = block->q_number;
        break;
    case G_74_1:
    case G_84_1:
        CHKS(((settings->motion_mode != motion) && (block->p_number == -1.0)),
             NCE_P_WORD_MISSING_WITH_G74_G84);
        block->p_number =
            block->p_number == -1.0 ? settings->cycle.p : block->p_number;
        CYCLE_MACRO(convert_cycle_g74_g84(cycle_end, plane, aa, bb, clear_cc, cc,
                                          settings->spindle_turning,
                                          settings->speed_feed_mode,
                                          motion, block->p_number))
        settings->cycle.p = block->p_number;
        break;
    case G_85:
        CYCLE_MACRO(convert_cycle_g85(cycle_end, plane, aa, bb, r, clear_cc, cc))
        break;
    case G_86:
        CHKS(((settings->motion_mode != G_86) && (block->p_number == -1.0)),
             NCE_DWELL_TIME_P_WORD_MISSING_WITH_G86);
        block->p_number =
            block->p_number == -1.0 ? settings->cycle.p : block->p_number;
        CYCLE_MACRO(convert_cycle_g86(cycle_end, plane, aa, bb, clear_cc, cc,
                                      block->p_number,
                                      settings->spindle_turning)) settings->
            cycle.p = block->p_number;
        break;
    case G_87:
        if (settings->motion_mode != G_87) {
            CHKS((!block->i_flag), NCE_I_WORD_MISSING_WITH_G87);
            CHKS((!block->j_flag), NCE_J_WORD_MISSING_WITH_G87);
            CHKS((!block->k_flag), NCE_K_WORD_MISSING_WITH_G87);
        }
        i = block->i_flag ? block->i_number : settings->cycle.i;
        j = block->j_flag ? block->j_number : settings->cycle.j;
        k = block->k_flag ? block->k_number : settings->cycle.k;
        settings->cycle.i = i;
        settings->cycle.j = j;
        settings->cycle.k = k;
        if (get_distance_mode(block, settings) == MODE_INCREMENTAL) {
            k = (cc + k);             /* k always absolute in function call below */
        }
        CYCLE_MACRO(convert_cycle_g87(cycle_end, plane, aa, (aa + i), bb,
                                      (bb + j), r, clear_cc, k, cc,
                                      settings->spindle_turning)) break;
    case G_88:
        CHKS(((settings->motion_mode != G_88) && (block->p_number == -1.0)),
             NCE_DWELL_TIME_P_WORD_MISSING_WITH_G88);
        block->p_number =
            block->p_number == -1.0 ? settings->cycle.p : block->p_number;
        CYCLE_MACRO(convert_cycle_g88(cycle_end, plane, aa, bb, cc,
                                      block->p_number,
                                      settings->spindle_turning)) settings->
            cycle.p = block->p_number;
        break;

    case G_89:
        CHKS(((settings->motion_mode != G_89) && (block->p_number == -1.0)),
             NCE_DWELL_TIME_P_WORD_MISSING_WITH_G89);
        block->p_number =
            block->p_number == -1.0 ? settings->cycle.p : block->p_number;
        CYCLE_MACRO(convert_cycle_g89(cycle_end, plane, aa, bb, clear_cc, cc,
                                      block->p_number))
        settings->cycle.p = block->p_number;
        break;
    default:
        ERS(NCE_BUG_FUNCTION_SHOULD_NOT_HAVE_BEEN_CALLED);
    }
    settings->current = cycle_end; // For non-cycle axes
    settings->current[axis_0] = aa;     /* CYCLE_MACRO updates aa and bb */
    settings->current[axis_1] = bb;
    settings->current[depth_axis] = clear_cc;
    settings->cycle.cc = block->axis_numbers[depth_axis];

    if (save_mode != CANON_EXACT_PATH) {
        SET_MOTION_CONTROL_MODE(save_mode, save_tolerance);
    }

    return INTERP_OK;
}



/**
 * Converts cycle coordinates to an actual goal position usable for
 * STRAIGHT_FEED or STRAIGHT_TRAVERSE, given a block and active plane.
 *
 * This works by pulling out the nominal goal position from the block (using
 * all axes), but replacing the "active" canned cycle axis words with the given
 * cycle values.
 */
PositionArray Interp::get_cycle_position(PositionArray const &cycle_end,
                       CANON_PLANE plane,        //!< currently selected plane
                       double plane_x,      //!< first coordinate value
                       double plane_y,      //!< second coordinate value
                       double height)      //!< third coordinate value
{
    // Start with the nominal end position for the cycle
    PositionArray goal_pos = cycle_end;

    // Substitute the correct position based on the local cycle coordinates
    size_t plane_x_idx, plane_y_idx, height_idx; std::tie(plane_x_idx, plane_y_idx, height_idx) = plane_indices(plane);
    goal_pos[plane_x_idx] = plane_x;
    goal_pos[plane_y_idx] = plane_y;
    goal_pos[height_idx] = height;
    return goal_pos;
}


/**
 * cycle_feed

Returned Value: int (INTERP_OK)

Side effects:
  STRAIGHT_FEED is called.

Called by:
  convert_cycle_g81
  convert_cycle_g82
  convert_cycle_g83
  convert_cycle_g74_g84
  convert_cycle_g85
  convert_cycle_g86
  convert_cycle_g87
  convert_cycle_g88
  convert_cycle_g89

This writes a STRAIGHT_FEED command appropriate for a cycle move with
respect to the given plane. Only motions from the "active" axis set are allowed
(either XYZ or UVW depending on plane selection). ABC axis motion is never allowed during a cycle feed.

*/

int Interp::cycle_feed(PositionArray const &cycle_end,
                       CANON_PLANE plane,        //!< currently selected plane  
                       double plane_x,      //!< first coordinate value
                       double plane_y,      //!< second coordinate value
                       double height)      //!< third coordinate value
{
    auto goal_pos = get_cycle_position(cycle_end, plane, plane_x, plane_y, height);
    STRAIGHT_FEED(_setup.get_toplevel_line_number(),
                      UNPACK_POSITION_ARRAY_REF(goal_pos));
    return INTERP_OK;
}

/****************************************************************************/

/*! cycle_traverse

Returned Value: int (INTERP_OK)

Side effects:
  STRAIGHT_TRAVERSE is called.

Called by:
  convert_cycle
  convert_cycle_g81
  convert_cycle_g82
  convert_cycle_g83
  convert_cycle_g86
  convert_cycle_g87
  convert_cycle_xy (via CYCLE_MACRO)
  convert_cycle_yz (via CYCLE_MACRO)
  convert_cycle_zx (via CYCLE_MACRO)

This writes a STRAIGHT_TRAVERSE command appropriate for a cycle
move with respect to the given plane. Motion is ALLOWED in all axes except for the "depth" axis (e.g. XYABCUVW for G17).
*/

int Interp::cycle_traverse(PositionArray const &cycle_end,
                           CANON_PLANE plane,    //!< currently selected plane 
                           double plane_x,  //!< first coordinate value
                           double plane_y,  //!< second coordinate value
                           double height)  //!< third coordinate value
{
    auto goal_pos = get_cycle_position(cycle_end, plane, plane_x, plane_y, height);

    STRAIGHT_TRAVERSE(_setup.get_toplevel_line_number(),
                      UNPACK_POSITION_ARRAY_REF(goal_pos));
    return INTERP_OK;
}
