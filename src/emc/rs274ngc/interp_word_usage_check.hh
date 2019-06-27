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

enum QWordMeanings
{
    Q_SPLINE_Y_OFFSET, // G5
    Q_TOOL_ORIENTATION, // G10 tool orientation
    Q_NAIVE_CAM_TOLERANCE, // G64
    Q_PECK_DISTANCE, // G73, G83
    Q_COMPOUND_SLIDE_ANGLE, //G76
    Q_TOOL_NUMBER, //G76
    Q_TIMEOUT, //M66
    Q_ANALOG_VALUE, //M67/M68
    Q_USER,
    MAX_QWORD_MEANINGS
};

enum RWordMeanings
{
    R_RADIUS, // G2 / G3
    R_TOOL_RADIUS, // G10
    R_DEPTH_DEGRESSION, // G76
    R_RETRACT_HEIGHT, // canned cycles
    R_ORIENT_POSITION, // M19
    R_USER, // M-codes
    MAX_RWORD_MEANINGS
};

WordUsageStatus is_p_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes, int o_type);

WordUsageStatus is_q_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes);

WordUsageStatus is_r_allowed(const std::array<GCodes, GM_MAX_MODAL_GROUPS> & g_modes, GCodes motion_to_be, const std::array<int, M_MAX_MODAL_GROUPS> & m_modes);

#endif // INTERP_WORD_USAGE_CHECK_HH
