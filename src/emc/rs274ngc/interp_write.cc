/********************************************************************
* Description: interp_write.cc
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

/****************************************************************************/
/*! Update active g codes in the given modal state.

Returned Value: int (INTERP_OK)

Side effects:
   The active_g_codes in the settings are updated.

Called by:
   Interp::execute
   Interp::init

The block may be NULL.

This writes active g_codes into the settings->active_g_codes array by
examining the interpreter settings. The array of actives is composed
of ints, so (to handle codes like 59.1) all g_codes are reported as
ints ten times the actual value. For example, 59.1 is reported as 591.

The correspondence between modal groups and array indexes is as follows
(no apparent logic to it).

The group 0 entry is taken from the block (if there is one), since its
codes are not modal.

group 0  - gez[2]  g4, g10, g28, g30, g53, g92 g92.1, g92.2, g92.3 - misc
group 1  - gez[1]  g0, g1, g2, g3, g38.2, g80, g81, g82, g83, g84, g85,
                   g86, g87, g88, g89 - motion
group 2  - gez[3]  g17, g18, g19 - plane selection
group 3  - gez[6]  g90, g91 - distance mode
group 4  - gez[14] g90.1, g91.1 - IJK distance mode for arcs
group 5  - gez[7]  g93, g94, g95 - feed rate mode
group 6  - gez[5]  g20, g21 - units
group 7  - gez[4]  g40, g41, g42 - cutter radius compensation
group 8  - gez[9]  g43, g49 - tool length offse
group 9  - no such group
group 10 - gez[10] g98, g99 - return mode in canned cycles
group 11 - no such group
group 12 - gez[8]  g54, g55, g56, g57, g58, g59, g59.1, g59.2, g59.3
                   - coordinate system
group 13 - gez[11] g61, g61.1, g64 - control mode
group 14 - gez[12] g50, g51 - adaptive feed mode
group 15 - gez[13] g96, g97 - spindle speed mode
group 16 - gez[15] g7,g8 - lathe diameter mode

*/

int Interp::write_g_codes(block_pointer block,   //!< pointer to a block of RS274/NGC instructions
                         setup_pointer settings,
                         ModalState & state)        //!< pointer to machine settings
{
  state.g_codes[G_CODE_SEQUENCE_NUMBER] = settings->sequence_number;
  state.g_codes[G_CODE_MOTION_MODE] = settings->motion_mode;
  state.g_codes[G_CODE_MODE_0] = ((block == NULL) ? -1 : block->g_modes[0]);
  switch(settings->plane) {
  case CANON_PLANE_XY:
      state.g_codes[G_CODE_PLANE] = G_17;
      break;
  case CANON_PLANE_XZ:
      state.g_codes[G_CODE_PLANE] = G_18;
      break;
  case CANON_PLANE_YZ:
      state.g_codes[G_CODE_PLANE] = G_19;
      break;
  case CANON_PLANE_UV:
      state.g_codes[G_CODE_PLANE] = G_17_1;
      break;
  case CANON_PLANE_UW:
      state.g_codes[G_CODE_PLANE] = G_18_1;
      break;
  case CANON_PLANE_VW:
      state.g_codes[G_CODE_PLANE] = G_19_1;
      break;
  }
  state.g_codes[G_CODE_CUTTER_SIDE] =
    (settings->cutter_comp_side == RIGHT) ? G_42 :
    (settings->cutter_comp_side == LEFT) ? G_41 : G_40;
  state.g_codes[G_CODE_UNITS] = (settings->length_units == CANON_UNITS_INCHES) ? G_20 : G_21;
  state.g_codes[G_CODE_DISTANCE_MODE] = (settings->distance_mode == MODE_ABSOLUTE) ? G_90 : G_91;
  state.g_codes[G_CODE_FEED_MODE] = (settings->feed_mode == INVERSE_TIME) ? G_93 :
	    (settings->feed_mode == UNITS_PER_MINUTE) ? G_94 : G_95;
  state.g_codes[G_CODE_ORIGIN] =
    (settings->origin_index <
     7) ? (530 + (10 * settings->origin_index)) : (584 +
                                                   settings->origin_index);
  state.g_codes[G_CODE_TOOL_LENGTH_OFFSET] = (settings->tool_offset.tran.x || settings->tool_offset.tran.y || settings->tool_offset.tran.z ||
            settings->tool_offset.a || settings->tool_offset.b || settings->tool_offset.c ||
            settings->tool_offset.u || settings->tool_offset.v || settings->tool_offset.w) ? G_43 : G_49;
  state.g_codes[G_CODE_RETRACT_MODE] = (settings->retract_mode == OLD_Z) ? G_98 : G_99;
  state.g_codes[G_CODE_PATH_MODE] =
    (settings->control_mode == CANON_CONTINUOUS) ? G_64 :
    (settings->control_mode == CANON_EXACT_PATH) ? G_61 : G_61_1;
  state.g_codes[G_CODE_SPINDLE_MODE] =
    (settings->spindle_mode == CONSTANT_RPM) ? G_97 : G_96;
  state.g_codes[G_CODE_DISTANCE_MODE_IJK] = (settings->ijk_distance_mode == MODE_ABSOLUTE) ? G_90_1 : G_91_1;
  state.g_codes[G_CODE_LATHE_DIAMETER_MODE] = (settings->lathe_diameter_mode) ? G_7 : G_8;
  return INTERP_OK;
}

/****************************************************************************/

/*! Update the given modal state with current M codes.

Returned Value: int (INTERP_OK)

Called by:
   Interp::execute
   Interp::init

TODO This is testing only the feed override to see if overrides is on.
Might add check of speed override.

*/

int Interp::write_m_codes(block_pointer block,   //!< pointer to a block of RS274/NGC instructions
                         setup_pointer settings,
                         ModalState & state)        //!< pointer to machine settings
{
  state.m_codes[M_CODE_SEQUENCE_NUMBER] =   /* 0 seq number */
      settings->sequence_number;
  state.m_codes[M_CODE_STOPPING] =			/* 1 stopping */
      (block == NULL) ? -1 : block->m_modes[4];
  state.m_codes[M_CODE_SPINDLE] =			/* 2 spindle */
      (settings->spindle_turning == CANON_STOPPED) ? 5 :
      (settings->spindle_turning == CANON_CLOCKWISE) ? 3 : 4;
  state.m_codes[M_CODE_TOOLCHANGE] =			/* 3 tool change */
      (block == NULL) ? -1 : block->m_modes[6];
  state.m_codes[M_CODE_MIST] =				/* 4 mist */
      (settings->mist) ? 7 : (settings->flood) ? -1 : 9;
  state.m_codes[M_CODE_FLOOD] =				/* 5 flood */
      (settings->flood) ? 8 : -1;
  if (settings->feed_override) {			/* 6 overrides */
      if (settings->speed_override)
	  state.m_codes[M_CODE_OVERRIDES] =  48;
      else
	  state.m_codes[M_CODE_OVERRIDES] = 50;
  } else if (settings->speed_override) {
      state.m_codes[M_CODE_OVERRIDES] = 51;
  } else
      state.m_codes[M_CODE_OVERRIDES] = 49;
  
  state.m_codes[M_CODE_ADAPTIVE_FEED] =			/* 7 adaptive feed */
      (settings->adaptive_feed) ? 52 : -1;

  state.m_codes[M_CODE_FEED_HOLD] =			/* 8 feed hold */
      (settings->feed_hold) ? 53 : -1;

  return INTERP_OK;
}

/****************************************************************************/

/*! write_settings

Returned Value: int (INTERP_OK)

Side effects:
  The settings->active_settings array of doubles is updated with the
  sequence number, feed, and speed settings.

Called by:
  Interp::execute
  Interp::init

*/

int Interp::write_settings(setup_pointer settings,     //!< pointer to machine settings
        ModalState & state){

    // HERE
  state.settings[SETTINGS_SEQUENCE_NUMBER] =	/* 0 sequence number */
      settings->sequence_number;
  state.settings[SETTINGS_FEED_RATE] =		/* 1 feed rate */
      settings->feed_rate;
  state.settings[SETTINGS_SPEED] =		/* 2 spindle speed */
      settings->speed;

  return INTERP_OK;
}

int Interp::write_and_update_state_tag(block_pointer block)
{
    int offset, axis;
    StateTag state;

    if (_setup.skipping_to_sub) {
	logDebug("write_and_update_state_tag:  not tagging block while skipping");
	return 0;
    }
    state.fields[GM_FIELD_LINE_NUMBER] = _setup.get_toplevel_line_number();
    state.fields[GM_FIELD_STEPPING_ID] = _setup.stepping_id;
    //FIXME refactor these into setup methods, and maybe put this whole method in setup struct
    bool in_remap = (_setup.remap_level > 0);
    bool in_sub = (_setup.call_level > 0 && _setup.remap_level == 0);
    bool external_sub = strcmp(_setup.filename, _setup.sub_context[0].filename);

    state.flags[GM_FLAG_IN_REMAP] = in_remap;
    state.flags[GM_FLAG_IN_SUB] = in_sub;
    state.flags[GM_FLAG_EXTERNAL_FILE] = external_sub;
    // Tormach - change so that all tags are restorable.  This may change in future
    state.flags[GM_FLAG_RESTORABLE] = 1;
    state.fields[GM_FIELD_G_MODE_0] = ((block == NULL) ? -1 : block->g_modes[0]);
    state.fields[GM_FIELD_MOTION_MODE] = _setup.motion_mode;
    switch(_setup.plane) {
        case CANON_PLANE_XY:
            state.fields[GM_FIELD_PLANE] = G_17;
            break;
        case CANON_PLANE_XZ:
            state.fields[GM_FIELD_PLANE] = G_18;
            break;
        case CANON_PLANE_YZ:
            state.fields[GM_FIELD_PLANE] = G_19;
            break;
        case CANON_PLANE_UV:
            state.fields[GM_FIELD_PLANE] = G_17_1;
            break;
        case CANON_PLANE_UW:
            state.fields[GM_FIELD_PLANE] = G_18_1;
            break;
        case CANON_PLANE_VW:
            state.fields[GM_FIELD_PLANE] = G_19_1;
            break;
    }

  state.fields[GM_FIELD_CUTTER_COMP] =
    (_setup.cutter_comp_side == RIGHT) ? G_42 :
    (_setup.cutter_comp_side == LEFT) ? G_41 : G_40;

    state.flags[GM_FLAG_UNITS] = (_setup.length_units == CANON_UNITS_INCHES);

    state.flags[GM_FLAG_DISTANCE_MODE] = (_setup.distance_mode == MODE_ABSOLUTE);
    state.flags[GM_FLAG_FEED_INVERSE_TIME] = (_setup.feed_mode == INVERSE_TIME);
    state.flags[GM_FLAG_FEED_UPM] = (_setup.feed_mode == UNITS_PER_MINUTE);


    state.fields[GM_FIELD_ORIGIN] =
        (_setup.origin_index <
         7) ? (530 + (10 * _setup.origin_index)) : (584 +
             _setup.origin_index);

    state.flags[GM_FLAG_TOOL_OFFSETS_ON] = (_setup.tool_offset.tran.x || _setup.tool_offset.tran.y || _setup.tool_offset.tran.z ||
            _setup.tool_offset.a || _setup.tool_offset.b || _setup.tool_offset.c ||
            _setup.tool_offset.u || _setup.tool_offset.v || _setup.tool_offset.w);
    state.flags[GM_FLAG_RETRACT_OLDZ] = (_setup.retract_mode == OLD_Z);
    state.flags[GM_FLAG_BLEND] = (_setup.control_mode == CANON_CONTINUOUS);
    state.flags[GM_FLAG_EXACT_STOP] = (_setup.control_mode == CANON_EXACT_STOP);
    state.flags[GM_FLAG_CSS_MODE] = (_setup.spindle_mode == CONSTANT_RPM);
    state.flags[GM_FLAG_IJK_ABS] = (_setup.ijk_distance_mode == MODE_ABSOLUTE);
    state.flags[GM_FLAG_DIAMETER_MODE] = (_setup.lathe_diameter_mode);

    state.fields[GM_FIELD_M_MODES_4] = (block == NULL) ? -1 : block->m_modes[4];

    state.flags[GM_FLAG_SPINDLE_ON] = !(_setup.spindle_turning != CANON_STOPPED);
    state.flags[GM_FLAG_SPINDLE_CW] = (_setup.spindle_turning == CANON_CLOCKWISE);

    state.fields[GM_FIELD_TOOLCHANGE] = (block == NULL) ? -1 : block->m_modes[6];

    state.fields[GM_FIELD_POCKET_NUMBER] = (_setup.current_pocket);
    state.fields[GM_FIELD_TOOL_NUMBER] = (_setup.tool_table[_setup.current_pocket].toolno);

    state.flags[GM_FLAG_MIST] = (_setup.mist) ;
    state.flags[GM_FLAG_FLOOD] = (_setup.flood);

    state.flags[GM_FLAG_FEED_OVERRIDE] = _setup.feed_override;
    state.flags[GM_FLAG_SPEED_OVERRIDE] = _setup.speed_override;

    state.flags[GM_FLAG_ADAPTIVE_FEED] = (_setup.adaptive_feed);

    state.flags[GM_FLAG_FEED_HOLD] =  (_setup.feed_hold);

    state.feed = _setup.feed_rate;        /* 1 feed rate       */
    state.speed = _setup.speed;    /* 2 spindle speed   */

    for (offset=0; offset<9; offset++)
	for (axis=0; axis<9; axis++)
	    state.offsets[offset*9+axis] =
		_setup.parameters[5221 + offset*20 + axis];

    UPDATE_TAG(state);
    return 0;
}
    

/****************************************************************************/

