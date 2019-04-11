/********************************************************************
* Description:  units.h
*               Unit conversion macros and constants
*
* Derived from a work by Fred Proctor & Will Shackleford
*
* License: GPL Version 2
*    
* Copyright (c) 2005 All rights reserved.
*
* Last change: 
********************************************************************/

#ifndef UNITS_H
#define UNITS_H
#include <canon.hh>

static inline double TO_EXT_LEN(double mm)
{
    return mm * GET_EXTERNAL_LENGTH_UNITS();
}

static inline double TO_EXT_ANG(double deg)
{
    return deg * GET_EXTERNAL_ANGLE_UNITS();
}

static inline double FROM_EXT_LEN(double ext)
{
    return ext / GET_EXTERNAL_LENGTH_UNITS();
}

static inline double FROM_EXT_ANG(double ext)
{
    return ext / GET_EXTERNAL_ANGLE_UNITS();
}

#endif
