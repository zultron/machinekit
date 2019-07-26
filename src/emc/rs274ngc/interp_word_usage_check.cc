#include "interp_word_usage_check.hh"
#include "boost/config.hpp"
#include <algorithm>


template <typename T, size_t N>
WordUsageStatus count_usages(std::array<T, N> const &active_usages, bool have_unused_word)
{
    int usage_count = std::count(active_usages.begin(),
                                active_usages.end(),
                                true);
    if (usage_count > 1)
    {
        return WORD_MEANING_CONFLICT;
    } else if (usage_count == 1) {
        return have_unused_word ? WORD_IGNORED : WORD_USAGE_OK;
    }
    return WORD_NOT_ALLOWED;
}
WordUsageStatus is_p_allowed(std::array<GCodes, GM_MAX_MODAL_GROUPS> const &g_modes,
                             GCodes motion_to_be,
                             std::array<int, M_MAX_MODAL_GROUPS> const &m_modes,
                             int o_type
                             )
{
    std::array<bool, MAX_PWORD_MEANINGS> active_p{};
    // Flag set to record what P word meanings are active based on the current settings and block
    bool unused_p = false;

    switch (motion_to_be)
    {
    // Mode 1 commands that use
    case G_2:
    case G_3:
        active_p[P_TURNS] = true;
        break;
    case G_5:
        active_p[P_SPLINE_X_OFFSET] = true;
        break;
    case G_5_2:
        active_p[P_NURBS_WEIGHT] = true;
        break;
    case G_38_2:
    case G_38_3:
    case G_38_4:
    case G_38_5:
    case G_38_6:
        active_p[P_PROBE_TO_LIMIT] = true;
        break;
    case G_76:
        active_p[P_THREAD_PITCH] = true;
        break;
    case G_81:
    case G_82:
        unused_p=true;
        BOOST_FALLTHROUGH;
    case G_83:
    case G_86:
    case G_88:
    case G_89:
        active_p[P_DWELL_TIME] = true;
        break;
    default:
        break;
    }

    if (g_modes[GM_COORD_SYSTEM] == G_54_1) {
        active_p[P_OFFSET_INDEX] = true;
    }

    if (g_modes[GM_CONTROL_MODE] == G_64) {
        active_p[P_BLEND_TOLERANCE] = true;
    }

    switch (g_modes[GM_MODAL_0])
    {
    // Mode 1 commands that use
    case G_4:
        active_p[P_DWELL_TIME] = true;
        break;
    case G_10:
        active_p[P_OFFSET_INDEX] = true;
        break;
    default:
        break;
    }

    if (m_modes[M_MODE_SPINDLE] == 19) {
        active_p[P_ORIENT_DIRECTION] = true;
    }
    switch (m_modes[M_MODE_FEED_SPEED_OVERRIDE])
    {
    case 50:
    case 51:
    case 52:
    case 53:
        active_p[P_OVERRIDE_ACTIVE]=true;
        // Feed / speed overrides use P wor
    }
    switch (m_modes[M_MODE_IO])
    {
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
        active_p[P_IO_PIN]=true;
    }

    active_p[P_USER_MCODE] = m_modes[M_MODE_USER] > 0;
    active_p[P_SUBROUTINE_NUMBER] = o_type == M_98;

    return count_usages(active_p, unused_p);
}

/**
 * Looks for meaning conflicts between Q words in a G-code line based on the active modes from a block.
 */
WordUsageStatus is_q_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes)
{
    std::array<bool, MAX_QWORD_MEANINGS> active_q{};
    switch (motion_to_be)
    {
    case G_5:
        active_q[Q_SPLINE_Y_OFFSET] = true;
        break;
    case G_73:
    case G_83:
        active_q[Q_PECK_DISTANCE] = true;
        break;
    case G_76:
        active_q[Q_COMPOUND_SLIDE_ANGLE] = true;
        break;
    default:
        break;
    }

    if (g_modes[GM_MODAL_0] == G_10) {
        active_q[Q_TOOL_ORIENTATION] = true;
    }
    if (g_modes[GM_CONTROL_MODE] == G_64) {
        active_q[Q_NAIVE_CAM_TOLERANCE] = true;
    }

    switch (m_modes[M_MODE_IO])
    {
    case 66:
        active_q[Q_TIMEOUT] = true;
        break;
    case 67:
    case 68:
        active_q[Q_ANALOG_VALUE] = true;
        break;
    }

    if (m_modes[M_MODE_TOOLCHANGE] == 61) { active_q[Q_TOOL_NUMBER] = true;}
    if (m_modes[M_MODE_SPINDLE] == 19) { active_q[Q_TIMEOUT] = true;}
    if (m_modes[M_MODE_USER] >= 100) { active_q[Q_USER] = true;}

    return count_usages(active_q, false);
}

WordUsageStatus is_r_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes)
{
    std::array<bool, MAX_RWORD_MEANINGS> active_r{};
    switch (motion_to_be)
    {
    case G_2:
    case G_3:
        active_r[R_RADIUS] = true;
        break;
    case G_76:
        active_r[R_DEPTH_DEGRESSION] = true;
        break;
    case G_73:
    case G_81:
    case G_82:
    case G_83:
    case G_84:
    case G_85:
    case G_86:
    case G_87:
    case G_88:
    case G_89:
        active_r[R_RETRACT_HEIGHT] = true;
        break;
    default:
        break;
    }

    switch (g_modes[GM_MODAL_0]) {
    case G_10:
        active_r[R_TOOL_RADIUS] = true;
        break;
    default:
        break;
    }

    if (m_modes[M_MODE_SPINDLE] == 19) { active_r[R_ORIENT_POSITION] = true;}

    return count_usages(active_r, false);
}
