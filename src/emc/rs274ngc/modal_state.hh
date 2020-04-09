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
#include <array>
#include <interp_fwd.hh>
#include <interp_enums.hh>
#include <coordinatetransform.hh>

/**
 * Captures modal state and settings for cutter compensation for exact state restore via generated G-code.
 */
struct CutterCompData
{
    double radius;    // current cutter compensation radius
    int orientation;  // current cutter compensation tool orientation
    CutterCompDirection side;         // current cutter compensation side
    bool firstmove;   // this is the first comp move
    bool radius_from_tool;
    int tool_number;
};

/**
 * Captures "modal" state (the various G / M modes and associated settings for each mode like feed and speed).
 *
 * @note This has mutated somewhat recently to contain more state details used
 * for state restore (i.e. M72 or after abort). It should be refactored
 * eventually to isolate the truely modal state from the miscellaneous data
 * that's convenenient to carry along.
 */
struct ModalState {
    ModalState() = default;
    ActiveGCodeArray g_codes;       // array of active G codes
    ActiveMCodeArray m_codes;      // array of active M codes
    ActiveSettingsArray settings;     // array of feed, speed, etc.
    int work_offset_number;
    int tool_number;
    int tool_offset_number;
    GCodes tool_offset_mode;
    CutterCompData cutter_comp;
    double feed_rate_uu;
};

/**
 * Configuration for an auto-run (open and run a program).
 * This expands on the original auto-run command (which just specified the
 * start line). Now it includes other settings to control how modal state is
 * handled when running from a line other than the start.
 */
struct AutoRunSettings
{
    AutoRunSettings() :
        start_line(0),
        leadin_type(0),
        keep_current_tool(true)
    {}
    int start_line; //!< line in main program where user wants to start
    int leadin_type; //!< Controls lead-in behavior when not starting from the beginning of the program
    bool keep_current_tool;
};


/**
 * A larger set of state that is used to set up Interp when running from a line other than the first.
 *
 * Capturing additional state like this beyond strictly modal state is
 * necessary to accurately handle the edge cases, to fully restore the program
 * state as it would have been if it was run from the beginning.
 */
struct ExpandedModalState {
    PositionArray initial_tool_position_uu; //!< Theoretical initial machine position if the spindle nose were at the same height as the current tool tip
    ShiftTransform tool_offset_uu; //!< currently applied custom tool offset (if applicable)
    ModalState modal_state;
};

#endif
