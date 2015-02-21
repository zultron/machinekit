/********************************************************************
* Description: cms_pm.cc
*   Provides CMS update functions for the POSEMATH classes.*
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: LGPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change: 
********************************************************************/

#include "cms.hh"		// class CMS
#include "state_tag.h"
// translation types
CMS_STATUS CMS::update(state_tag_t &tag)
{
    update(tag.feed);
    update(tag.speed);
    update(tag.fields, GM_FIELD_MAX_FIELDS);
    update(tag.packed_flags);
    return (status);
}
