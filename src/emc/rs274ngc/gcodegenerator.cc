#include "gcodegenerator.hh"
#include "config.h"
#include "interp_internal.hh"
#include <sstream>

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
        snprintf(buf, sizeof(buf), "G%d.%d", base_g, sub_g);
    } else {
        snprintf(buf, sizeof(buf), "G%d", base_g);
    }

    cmd += buf;

    return 0;
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

int GCodeGenerator::filterAbortedStates(ModalState & current, ModalState & saved)
{
    // Clean up the modal state as needed for the special case of on-abort restore:
    // Force cancellation of tool compensation
    saved.g_codes[G_CODE_CUTTER_SIDE] = G_40;

    // KLUDGE force restoration of any modal state that isn't fully expressed by just the mode (e.g. tool number, offset number)
    current.g_codes[G_CODE_TOOL_LENGTH_OFFSET] = -1;
    current.g_codes[G_CODE_ORIGIN] = -1;

    // Force M-modes to match so they are never restored;
    saved.m_codes = current.m_codes;
    return 0;
}

void GCodeGenerator::addWorkOffset(int work_offset_number)
{
    char buf[LINELEN];
    snprintf(buf, LINELEN, "G54.1 P%d", work_offset_number);
    cmd_list.emplace_back(buf);
}

int GCodeGenerator::addModalStateGCodes(ModalState & current, const ModalState & saved)
{
    // Always restore work offset number using extended work offset format (since modal state is not fully captured in the G5x mode)
    addWorkOffset(saved.work_offset_number);

    std::string cmd;
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
            break;

        case G_CODE_TOOL_LENGTH_OFFSET: // - tool offset (G43/G49)
            formatGCode((GCodes)val, cmd);
            if (has_feature(features, FEATURE_LATHE_TOOLCHANGE) && val == G_43) {
                char buf[LINELEN];
                snprintf(buf, sizeof(buf), "H%d", saved.tool_offset_number);
                cmd += buf;
            }
            break;

        case G_CODE_SPINDLE_MODE: // - spindle mode
            // Force restoration of spindle speed too
        // If G96 spindle mode is present for constant surface speed, you must have an S word
        // also even if S appears to be the same between saved and current vectors.
            current.settings[SETTINGS_SPEED] = -1;
            BOOST_FALLTHROUGH;
        case G_CODE_PLANE: // - plane
        case G_CODE_CUTTER_SIDE: // - cutter compensation
        case G_CODE_DISTANCE_MODE: // - distance mode
        case G_CODE_FEED_MODE: // - feed mode
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

    char buf[LINELEN]={};
    for (int ii = 0; ii < ACTIVE_SETTINGS; ii++) {
        if (saved.settings[ii] == current.settings[ii]) {
            continue;
        }
        switch ((ActiveSettingIndex)ii) {
        case SETTINGS_SEQUENCE_NUMBER: break; // sequence_number - no point in restoring
        case SETTINGS_FEED_RATE:
            snprintf(buf,sizeof(buf)," F%.1f", saved.settings[ii]);
            cmd+=buf;
            break;
        case SETTINGS_SPEED:
            snprintf(buf,sizeof(buf)," S%.0f", saved.settings[ii]);
            cmd+=buf;
            break;
        case ACTIVE_SETTINGS: // Not a real setting index
            break;
        }
    }
    cmd_list.emplace_back(cmd);
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

int GCodeGenerator::gen_modalStateRestore(ModalState & current, const ModalState & saved)
{
    if (current.g_codes[G_CODE_UNITS] !=
        saved.g_codes[G_CODE_UNITS]) {
        auto g_unit = (GCodes)saved.g_codes[G_CODE_UNITS];
        formatGCode(g_unit, cmd_list.back());
    }
    // Each M code needs to execute on its own line to avoid conflicts
    addModalStateMCodes(current.m_codes, saved.m_codes);
    start_next_line();
    // G code commands currently all execute in a single line (may change in the future if there are conflicts)
    addModalStateGCodes(current, saved);
    return 0;
}
