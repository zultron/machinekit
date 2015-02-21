/********************************************************************
* Description: state_tag.c
*
* A "tag" struct that is used to add interpreter state information to
* a given motion line. This state info isn't actually used by motion
* directly, but indicates the motion state.
*
* Author: Robert W. Ellenberg
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2015 All rights reserved
********************************************************************/

#include "state_tag.h"

int get_line_number(struct state_tag_t const * const tag)
{
    if (!tag) {
        return -1;
    }
    return tag->fields[GM_FIELD_LINE_NUMBER];
}

int get_stepping_id(struct state_tag_t const * const tag)
{
    if (!tag) {
        return -1;
    }
    return tag->fields[GM_FIELD_STEPPING_ID];
}

