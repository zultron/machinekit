//    Copyright 2008, Chris Radek <chris@timeguy.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#ifndef MOTION_TYPES_H
#define MOTION_TYPES_H

typedef enum {
    EMC_MOTION_TYPE_TRAVERSE = 1,
    EMC_MOTION_TYPE_FEED = 2,
    EMC_MOTION_TYPE_ARC = 3,
    EMC_MOTION_TYPE_TOOLCHANGE = 4,
    EMC_MOTION_TYPE_PROBING = 5,
    EMC_MOTION_TYPE_INDEXROTARY = 6,
} EMCMotionTypes;

typedef enum {
    SPINDLE_REVERSE=-1,
    SPINDLE_STOPPED=0,
    SPINDLE_FORWARD=1
} spindle_direction_code_t;

typedef enum {
    POS_TRACKING_INACTIVE,
    POS_TRACKING_SYNC_ACCEL,
    POS_TRACKING_ACTIVE
} position_tracking_state_t;

typedef enum {
    POS_TRACK_MODE_LEGACY,
    POS_TRACK_MODE_FLATTENED,
    POS_TRACK_MODE_TRAPEZOIDAL,
} position_tracking_mode_t;

typedef enum {
    RIGIDTAP_INACTIVE,
    RIGIDTAP_TAPPING,
    RIGIDTAP_REVERSING,
    RIGIDTAP_RETRACTION,
    RIGIDTAP_FINAL_REVERSAL,
    RIGIDTAP_FINAL_PLACEMENT
} rigid_tap_state_t;

#endif
