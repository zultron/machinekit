/********************************************************************
* Description: modal_state.cc
*
* State storage class for interpreter
*
* Author: Robert W. Ellenberg
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2014 All rights reserved.
*
********************************************************************/
#include "StateTag.hh"
#include <string.h>

StateTag::StateTag(): flags(0)
{
    memset(fields,-1,sizeof(fields));
    packed_flags = 0;
    feed = 0.0;
    speed = 0.0;
}

StateTag::StateTag(struct state_tag_t const & basetag): state_tag_t(basetag), flags(basetag.packed_flags)
{}

/**
 * Return true if the tag is a valid state, and false if not
 */
int StateTag::is_valid(void) const
{

    if (fields[GM_FIELD_LINE_NUMBER] < 0) {
        return false;
    }

    //TODO magic numbers
    if (fields[GM_FIELD_ORIGIN] < 540) {
        return false;
    }

    if (fields[GM_FIELD_PLANE] < 170 ) {
        return false;
    }

    return true;
}


/**
 * Return the C-equivalent state_tag version of the current state.
 */
state_tag_t StateTag::get_state_tag() const
{
    state_tag_t out = static_cast<state_tag_t> (*this);
    out.packed_flags = flags.to_ulong();
    return out;
}

void StateTag::fprint(FILE *stream)
{
    fprintf(stream, "State tag dump:  feed %.2f; speed %.2f\n", feed, speed);
    fprintf(stream, "  fields:  line_number %d; stepping_id %d; next_line %d\n",
	    fields[GM_FIELD_LINE_NUMBER], fields[GM_FIELD_STEPPING_ID],
	    fields[GM_FIELD_NEXT_LINE]);
    fprintf(stream, "           g_mode_0 %d; cutter_comp %d; motion_mode %d\n",
	    fields[GM_FIELD_G_MODE_0], fields[GM_FIELD_CUTTER_COMP],
	    fields[GM_FIELD_MOTION_MODE]);
    fprintf(stream, "           plane %d; m_modes_4 %d; origin %d; toolchange %d\n",
	    fields[GM_FIELD_PLANE], fields[GM_FIELD_M_MODES_4],
	    fields[GM_FIELD_ORIGIN], fields[GM_FIELD_TOOLCHANGE]);
    fprintf(stream, "  flags: %s\n", flags.to_string().c_str());
}
