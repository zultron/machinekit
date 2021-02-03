/**
 * @file gcodegenerator.cc
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#include <sstream>
#include <iomanip>

#include "gcodegenerator.hh"
#include "coordinatetransform.hh"

#include "config.h"

//TODO error checking consistently like with interp macros (or exceptions?)

void GCodeGenerator::start_next_line()
{
    cmd_list.push_back("");
}

int GCodeGenerator::formatGCode(GCodes g, std::string & cmd)
{
    if (g < 0 || g > 999) {
        return -1;
    }

    char buf[50];
    int base_g = g / 10;
    int sub_g = g - base_g*10;
    if (sub_g) {
        snprintf(buf, sizeof(buf), "G%d.%d ", base_g, sub_g);
    } else {
        snprintf(buf, sizeof(buf), "G%d ", base_g);
    }

    cmd += buf;

    return 0;
}

/**
 * Populate axis words in G-code parsable format.
 * Example (with axis mask XY, position [1.0,1e-16, ...] -> "X1.00000000 Y0.00000000"
 */
void GCodeGenerator::add_axes(std::ostream &out, PositionArray const &goal, AxisMask active_axes)
{
    for (size_t i = 0; i < goal.size(); ++i) {
        if (!axis_mask.test(i) || !active_axes.test(i)) {
            continue;
        }
        out << AxisNames[i] << std::fixed << std::setprecision(8) << goal[i] << " ";
    }
}

const RestoreCommandList & GCodeGenerator::getCommandSequence() const
{
    return cmd_list;
}

std::string GCodeGenerator::getCommandSequenceAsString() const
{
    std::ostringstream out;
    for (auto s : cmd_list) {
        out << s << "\n";
    }
    out.flush();
    return out.str();
}

void GCodeGenerator::filterAbortedStates(ModalState & current, ModalState & saved)
{
    // Clean up the modal state as needed for the special case of on-abort restore:
    // Force cancellation of tool compensation
    saved.g_codes[G_CODE_CUTTER_SIDE] = G_40;

    // KLUDGE force restoration of any modal state that isn't fully expressed by just the mode (e.g. tool number, offset number)
    current.g_codes[G_CODE_TOOL_LENGTH_OFFSET] = -1;
    current.g_codes[G_CODE_ORIGIN] = -1;

    // Force M-modes to match so they are never restored;
    saved.m_codes = current.m_codes;
}

void GCodeGenerator::addWorkOffset(int work_offset_number)
{
    char buf[LINELEN];
    snprintf(buf, LINELEN, "G54.1 P%d", work_offset_number);
    cmd_list.emplace_back(buf);
}

void GCodeGenerator::addToolOffsetUpdate(int tool_offset_number)
{
    char buf[LINELEN];
    // Does a no-op G10 to push the tool offset from interp's tool table down to task / IO
    snprintf(buf, LINELEN, "G10 L1 P%d", tool_offset_number);
    cmd_list.emplace_back(buf);
}

void GCodeGenerator::addToolChange(int tool_number)
{
    char buf[LINELEN];
    if (has_feature(features, FEATURE_LATHE_TOOLCHANGE)) {
        snprintf(buf, LINELEN, "T%d", tool_number);
    } else {
        snprintf(buf, LINELEN, "M6 T%d", tool_number);
    }
    cmd_list.emplace_back(buf);
}

void GCodeGenerator::addToolLengthOffsetFromTable(int tool_number)
{
    char buf[LINELEN];
    snprintf(buf, sizeof(buf), "G43 H%d", tool_number);
    cmd_list.push_back(buf);
}

/**
 * Directly set up local axis offsets (equivalent to G52 and axis words).
 */
void GCodeGenerator::addLocalAxisOffsets(const ShiftTransform & local_axis_offset, AxisMask active_axes)
{
    if (!active_axes.any()) {
        return;
    }
    std::ostringstream gcode;
    gcode << "G52 ";
    add_axes(gcode, local_axis_offset.tf, active_axes);
    cmd_list.push_back(gcode.str());
}

void GCodeGenerator::clearLocalAxisOffsets(bool clear_params)
{
    cmd_list.push_back(clear_params ? "G92.1" : "G92.2");
}

void GCodeGenerator::addToolCompensationWithRadius(GCodes cutter_side, double cutter_radius)
{
    start_next_line();
    std::string &cmd = cmd_list.back();
    formatGCode((GCodes)(cutter_side + 1), cmd);
    char buf[LINELEN];
    snprintf(buf, sizeof(buf), "D%f", cutter_radius * 2.0);
    cmd += buf;
}

void GCodeGenerator::addToolCompensationFromTable(GCodes cutter_side, int tool_number)
{
    start_next_line();
    std::string &cmd = cmd_list.back();
    formatGCode((GCodes)(cutter_side), cmd);
    if (tool_number >= 0) {
        char buf[LINELEN];
        snprintf(buf, sizeof(buf), "D%d", tool_number);
        cmd += buf;
    }
}

void GCodeGenerator::printMsg(const std::string && msg)
{
    cmd_list.emplace_back("(PRINT,"+msg+")");
}

void GCodeGenerator::addSpindleMode(GCodes spindle_mode, double css_max_speed, double speed)
{
    std::string cmd;
    char buf[LINELEN]={};
    formatGCode(spindle_mode, cmd);
    if (spindle_mode == G_96) {
        snprintf(buf,sizeof(buf),"D%.17f ", css_max_speed);
        cmd+=buf;
    }
    // Always include a spindle speed (even if it's unchanged) when setting spindle mode
    snprintf(buf,sizeof(buf),"S%.17f", speed);
    cmd+=buf;
    cmd_list.emplace_back(cmd);
}

void GCodeGenerator::addSpindleSpeed(double speed)
{
    std::ostringstream speed_word;
    speed_word << " S" << std::fixed << std::setprecision(8) << speed;
    cmd_list.emplace_back(speed_word.str());
}

int GCodeGenerator::addModalStateGCodes(ModalState & current, const ModalState & saved)
{
    // Always restore work offset number using extended work offset format (since modal state is not fully captured in the G5x mode)
    addWorkOffset(saved.work_offset_number);

    std::string cmd; // Main command sequence to restore modal state

    for (int ii = 0; ii < ACTIVE_G_CODES; ii++) {
        int val = saved.g_codes[ii];
        if (val == current.g_codes[ii]) {
            continue;
        }

        switch ((ActiveGCodeIndex)ii) {
        case G_CODE_SEQUENCE_NUMBER: // sequence_number - no point in restoring
        case G_CODE_MODE_0: // TODO document why we don't restore mode 0
        case G_CODE_unsupported: // unused
        case G_CODE_UNITS: // - length units (handled separately)
        case G_CODE_MOTION_MODE: // restoring the motion mode is a real bad idea to start with
        case G_CODE_ORIGIN: // Handled separately due to extended work offsets
        case G_CODE_SPINDLE_MODE: // Handled separately (needs to occur after all offsets are updated for G96)
        case G_CODE_FEED_MODE: // Handled separately (called together with feed rate for clarity)
            break;

        case G_CODE_TOOL_LENGTH_OFFSET: // - tool offset (G43/G49)
            formatGCode((GCodes)val, cmd);
            if (has_feature(features, FEATURE_LATHE_TOOLCHANGE) && val == G_43) {
                char buf[LINELEN];
                //FIXME handle a negative H
                snprintf(buf, sizeof(buf), "H%d ", saved.tool_offset_number);
                cmd += buf;
            }
            break;

        case G_CODE_PLANE: // - plane
        case G_CODE_CUTTER_SIDE: // - cutter compensation
        case G_CODE_DISTANCE_MODE: // - distance mode
        case G_CODE_RETRACT_MODE: // - retract mode
        case G_CODE_PATH_MODE: // - control mode
        case G_CODE_DISTANCE_MODE_IJK: // - ijk distance mode
        case G_CODE_LATHE_DIAMETER_MODE: // - lathe diameter mode
            formatGCode((GCodes)val, cmd);
            break;
        case ACTIVE_G_CODES: // Do nothing, not a real G-code
            break;
        }
    }

    cmd_list.emplace_back(cmd);

    // End of bulk G-code restore here. Now restore speed / feed modes

    addFeedMode((GCodes)saved.g_codes[G_CODE_FEED_MODE], saved.settings[SETTINGS_FEED_RATE]);

    if (saved.g_codes[G_CODE_SPINDLE_MODE] != current.g_codes[G_CODE_SPINDLE_MODE]) {
        GCodes spindle_mode = (GCodes)saved.g_codes[G_CODE_SPINDLE_MODE];
        addSpindleMode(
            spindle_mode,
            saved.settings[SETTINGS_CSS_MAX_SPEED],
            saved.settings[SETTINGS_SPEED]);
    } else if (saved.settings[SETTINGS_SPEED] != current.g_codes[SETTINGS_SPEED]) {
        addSpindleSpeed(saved.settings[SETTINGS_SPEED]);
    }

    return 0;
}

int GCodeGenerator::addModalStateMCodes(const ActiveMCodeArray & current, const ActiveMCodeArray & saved)
{
    int i,val;
    char buf[LINELEN];
    for (i = 0; i < ACTIVE_M_CODES; i++) {
        val = saved[i];
        if (val == current[i]) {
            continue;
        }

        switch ((ActiveMCodeIndex)i) {
        case M_CODE_SEQUENCE_NUMBER:
        case M_CODE_STOPPING:
        case M_CODE_TOOLCHANGE:
            break;
        case M_CODE_SPINDLE:
        case M_CODE_MIST:
        case M_CODE_FLOOD:
        case M_CODE_OVERRIDES:
        case M_CODE_ADAPTIVE_FEED:
        case M_CODE_FEED_HOLD:
            if (val > -1) {
                snprintf(buf,sizeof(buf),"M%d", val);
                cmd_list.push_back(buf);
            }
            break;
        case ACTIVE_M_CODES: // Do nothing, not a real M-code
            break;
        }
    }
    return 0;
}

int GCodeGenerator::addQueueBuster(const char *comment)
{
    cmd_list.emplace_back("M66 P0 L0 ");
    if (comment) {
        cmd_list.back()+= "(";
        cmd_list.back()+= comment;
        cmd_list.back()+= ")";
    }
    return 0;
}

int GCodeGenerator::addLinearFeedMove(PositionArray const & goal, double feed_prog, AxisMask active_axes)
{
    std::ostringstream feed_move;
    feed_move << "G90.2 G1 ";
    add_axes(feed_move, goal, active_axes);

    if (feed_prog >= 0.0) {
        feed_move << "F" << feed_prog;
    }

    cmd_list.emplace_back(feed_move.str());
    return 0;
}

int GCodeGenerator::addLinearRapidMove(PositionArray const & goal, AxisMask active_axes)
{
    std::ostringstream rapid_move;
    rapid_move << "G90.2 G0 ";
    add_axes(rapid_move, goal, active_axes);

    cmd_list.emplace_back(rapid_move.str());
    return 0;
}

int GCodeGenerator::addFeedRate(double feed_rate)
{
    std::ostringstream feed_word;
    feed_word << "F" << std::fixed << std::setprecision(8) << feed_rate;

    cmd_list.emplace_back(feed_word.str());
    return 0;
}

int GCodeGenerator::addFeedMode(GCodes feed_mode, double feed_rate)
{
    std::string mode_str;
    formatGCode(feed_mode, mode_str);
    std::ostringstream feed_mode_cmd;
    feed_mode_cmd << mode_str << "F" << std::fixed << std::setprecision(8) << feed_rate;

    cmd_list.emplace_back(feed_mode_cmd.str());
    return 0;
}

int GCodeGenerator::gen_modalStateRestore(ModalState & current, const ModalState & saved)
{
    if (current.g_codes[G_CODE_UNITS] !=
        saved.g_codes[G_CODE_UNITS]) {
        start_next_line();
        auto g_unit = (GCodes)saved.g_codes[G_CODE_UNITS];
        formatGCode(g_unit, cmd_list.back());
    }
    // G code commands currently all execute in a single line (may change in the future if there are conflicts)
    addModalStateGCodes(current, saved);
    // Each M code needs to execute on its own line to avoid conflicts
    // Execute after the G-codes are set up so that the correct spindle speed is restored
    addModalStateMCodes(current.m_codes, saved.m_codes);
    return 0;
}

void GCodeGenerator::cancelToolCompensation()
{
    addPreformatted("G40");
}

void GCodeGenerator::addToolCompensation(GCodes cutter_side, CutterCompData const & comp_data_to_restore)
{
    if (comp_data_to_restore.side == NO_COMP) {
        cancelToolCompensation();
    } else if (comp_data_to_restore.radius_from_tool) {
        addToolCompensationFromTable(cutter_side, comp_data_to_restore.tool_number);
    } else {
        addToolCompensationWithRadius(cutter_side, comp_data_to_restore.radius);
    }
}

void GCodeGenerator::addPreformatted(const std::string & g_code_line, const char * trailing_comment_if_nonempty)
{
    std::ostringstream cmd;
    if (g_code_line.empty()) {return;}
    cmd << g_code_line;
    if (trailing_comment_if_nonempty && trailing_comment_if_nonempty[0]) {
        cmd << " (" << trailing_comment_if_nonempty << ")";
    }
    cmd_list.emplace_back(cmd.str());
}
