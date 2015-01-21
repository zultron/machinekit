/********************************************************************
* Description: modal_state.hh
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

#ifndef MODAL_STATE_HH
#define MODAL_STATE_HH
#include <vector>
#include <deque>
#include <bitset>
struct ModalState {
    ModalState();
    std::vector<int> g_codes;       // array of active G codes
    std::vector<int> m_codes;      // array of active M codes
    std::vector<double> settings;     // array of feed, speed, etc.
};

struct ModalStateHistory {
    ModalStateHistory();
    std::deque<ModalState> history;
};


/**
 * Enum to define bit names for StateTag's flags register.
 * The actual position of the flag isn't important, so the actual index doesn't
 * matter. However, the bit field should be <=64 bits to fit within a long int.
 */
typedef enum {
    GM_FLAG_UNITS,
    GM_FLAG_DISTANCE_MODE,
    GM_FLAG_TOOL_OFFSETS_ON,
    GM_FLAG_RETRACT_OLDZ,
    GM_FLAG_BLEND,
    GM_FLAG_EXACT_STOP,
    GM_FLAG_FEED_INVERSE_TIME,
    GM_FLAG_FEED_UPM,
    GM_FLAG_CSS_MODE,
    GM_FLAG_IJK_ABS,
    GM_FLAG_DIAMETER_MODE,
    GM_FLAG_SPINDLE_ON,
    GM_FLAG_SPINDLE_CW,
    GM_FLAG_MIST,
    GM_FLAG_FLOOD,
    GM_FLAG_FEED_OVERRIDE,
    GM_FLAG_SPEED_OVERRIDE,
    GM_FLAG_ADAPTIVE_FEED,
    GM_FLAG_FEED_HOLD,
    GM_FLAG_MAX_FLAGS
} StateFlag;

typedef enum {
    GM_FIELD_LINE_NUMBER,
    GM_FIELD_G_MODE_0,
    GM_FIELD_CUTTER_COMP,
    GM_FIELD_MOTION_MODE,
    GM_FIELD_PLANE,
    GM_FIELD_M_MODES_4,
    GM_FIELD_ORIGIN,
    GM_FIELD_TOOLCHANGE,
    GM_FIELD_MAX_FIELDS
} StateField;

/**
 * Tag structure that is added to a motion segment so that motion has a copy of
 * the relevant interp state.
 *
 * Previously, this information was stored only in the interpreter, and as
 * vectors of g codes, m codes, and settings. Considering that the write_XXX
 * and gen_XXX functions had to jump through hoops to translate from a settings
 * struct, the extra packing here isn't much more complex to deal with, and
 * will cost much less space in an NML message.
 * 
 */
struct StateTag {
    StateTag();
    // Machine settings
    // TODO do these have to be double?
    float feed;
    float speed;
    //TODO add others here if need be

    // Any data that doesn't pack nicely into a single bit
    std::vector<int> fields;

    // G / M mode flags (i.e. simple states like inch / mm, feedhold enable, etc.
    // This field should be no larger than a long int, so that we can easily
    // pack / unpack into a single long int in NML.  This restriction won't
    // matter once NML is repalced with ZMQ and protobufs.
    std::bitset<64> flags;
};
#endif
