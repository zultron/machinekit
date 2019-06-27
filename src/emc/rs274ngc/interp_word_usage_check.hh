#ifndef INTERP_WORD_USAGE_CHECK_HH
#define INTERP_WORD_USAGE_CHECK_HH

#include <interp_fwd.hh>
#include <array>

enum WordUsageStatus
{
    WORD_NOT_ALLOWED = -2,
    WORD_MEANING_CONFLICT = -1,
    WORD_USAGE_OK = 0,
    WORD_IGNORED = 1,
};


enum PWordMeanings
{
    P_DWELL_TIME, // G4, G8x canned cycles
    P_THREAD_PITCH, // G76
    P_PROBE_TO_LIMIT, // G38
    P_TURNS, // G2 and G3
    P_SPLINE_X_OFFSET,
    P_NURBS_WEIGHT,
    P_OFFSET_INDEX,
    P_OVERRIDE_ACTIVE,
    P_IO_PIN,
    P_USER_MCODE,
    P_SUBROUTINE_NUMBER,
    MAX_PWORD_MEANINGS
};

WordUsageStatus is_p_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes, int o_type);

#endif // INTERP_WORD_USAGE_CHECK_HH
