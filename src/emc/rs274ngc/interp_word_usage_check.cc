#include "interp_word_usage_check.hh"
#include "boost/config.hpp"
#include <algorithm>


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

    int p_meanings = std::count(active_p.begin(),
                                active_p.end(),
                                true);

    if (p_meanings > 1)
    {
        return WORD_MEANING_CONFLICT;
    } else if (p_meanings == 1) {
        return unused_p ? WORD_IGNORED : WORD_USAGE_OK;
    }

    return WORD_NOT_ALLOWED;
}
