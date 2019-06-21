/**
 * @file canon_enums.hh
 * 
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#ifndef CANON_ENUMS_HH
#define CANON_ENUMS_HH

enum CanonBool {
    OFF,
    ON
};

enum CANON_PLANE
{
    CANON_PLANE_XY = 1,
    CANON_PLANE_YZ,
    CANON_PLANE_XZ,
    CANON_PLANE_UV,
    CANON_PLANE_VW,
    CANON_PLANE_UW,
};

enum CANON_UNITS
{
    CANON_UNITS_INVALID = 0,
    CANON_UNITS_INCHES = 1,
    CANON_UNITS_MM,
    CANON_UNITS_CM,
};

enum CANON_MOTION_MODE
{
    CANON_EXACT_STOP = 1,
    CANON_EXACT_PATH,
    CANON_CONTINUOUS,
};

enum CANON_SPEED_FEED_MODE {
    CANON_SYNCHED = 1,
    CANON_INDEPENDENT,
};

enum CANON_DIRECTION {
    CANON_STOPPED = 1,
    CANON_CLOCKWISE,
    CANON_COUNTERCLOCKWISE,
};

enum CANON_FEED_REFERENCE {
    CANON_WORKPIECE = 1,
    CANON_XYZ,
};

enum CANON_SIDE
{
    CANON_SIDE_RIGHT = 1,
    CANON_SIDE_LEFT,
    CANON_SIDE_OFF,
};

enum CANON_AXIS
{
    CANON_AXIS_X = 1,
    CANON_AXIS_Y,
    CANON_AXIS_Z,
    CANON_AXIS_A,
    CANON_AXIS_B,
    CANON_AXIS_C,
    CANON_AXIS_U,
    CANON_AXIS_V,
    CANON_AXIS_W,
};

enum CANON_INPUT
{
    ANALOG_INPUT,
    DIGITAL_INPUT,
};

enum CANON_WAIT_MODE
{
    WAIT_MODE_IMMEDIATE=0,
    WAIT_MODE_RISE = 1,
    WAIT_MODE_FALL = 2,
    WAIT_MODE_HIGH = 3,
    WAIT_MODE_LOW = 4,
};

#endif // CANON_ENUMS_HH
