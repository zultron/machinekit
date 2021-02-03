#ifndef INTERP_ENUMS_HH
#define INTERP_ENUMS_HH

/*
 * Enum for `g_codes` indices, as written by `active_modes()`, and
 * similar int arrays used in other places like gcodemodule.cc
 *
 * WARNING:  This must start at 0, be monotonic, and ACTIVE_G_CODES
 * MUST be last
 */
typedef enum {
    G_CODE_SEQUENCE_NUMBER=0,
    G_CODE_MOTION_MODE,
    G_CODE_MODE_0, // Optional, from block
    G_CODE_PLANE,
    G_CODE_CUTTER_SIDE,
    G_CODE_UNITS,
    G_CODE_DISTANCE_MODE,
    G_CODE_FEED_MODE,
    G_CODE_ORIGIN,
    G_CODE_TOOL_LENGTH_OFFSET,
    G_CODE_RETRACT_MODE,
    G_CODE_PATH_MODE,
    G_CODE_unsupported,    // This slot in the original array was always set to -1 so assuming it really isn't supported.
                           // but some python programs that index into the status.gcodes array may depend on the array index
                           // so leaving this here for backwards compatibility.
                           // Newer python code should always use these same constant names, e.g. linuxcnc.G_CODE_ORIGIN
    G_CODE_SPINDLE_MODE,
    G_CODE_DISTANCE_MODE_IJK,
    G_CODE_LATHE_DIAMETER_MODE,
    ACTIVE_G_CODES
} ActiveGCodeIndex;

/*
 * Enum for `m_codes` indices, as written by `active_modes()`, and
 * similar int arrays used in other places like gcodemodule.cc
 *
 * WARNING:  This must start at 0, be monotonic, and ACTIVE_M_CODES
 * MUST be last
 */
typedef enum {
    M_CODE_SEQUENCE_NUMBER=0,
    M_CODE_STOPPING,
    M_CODE_SPINDLE,
    M_CODE_TOOLCHANGE,
    M_CODE_MIST,
    M_CODE_FLOOD,
    M_CODE_OVERRIDES,
    M_CODE_ADAPTIVE_FEED,
    M_CODE_FEED_HOLD,
    ACTIVE_M_CODES
} ActiveMCodeIndex;

/*
 * Enum for `settings` indices, as written by `active_modes()`, and
 * similar int arrays used in other places like gcodemodule.cc
 *
 * WARNING:  This must start at 0, be monotonic, and ACTIVE_SETTINGS
 * MUST be last
 */
typedef enum {
    SETTINGS_SEQUENCE_NUMBER=0,
    SETTINGS_FEED_RATE,
    SETTINGS_SPEED,
    SETTINGS_CSS_MAX_SPEED,
    ACTIVE_SETTINGS
} ActiveSettingIndex;

#ifdef __cplusplus
#include <array>

// Has to be int because CMS can't handle enums currently...
typedef std::array<int, ACTIVE_G_CODES> ActiveGCodeArray;
typedef std::array<int, ACTIVE_M_CODES> ActiveMCodeArray;
typedef std::array<double, ACTIVE_SETTINGS> ActiveSettingsArray;
#endif

// indicates which type of Python handler yielded, and needs reexecution
// post sync/read_inputs
enum RemapCallStates {
    CS_NORMAL,
    CS_REEXEC_PROLOG,
    CS_REEXEC_PYBODY,
    CS_REEXEC_EPILOG,
    CS_REEXEC_PYOSUB,
};

// detail for O_call; tags the frame
enum OCallTypes {
    CT_INVALID = -1,     // really not in a call...
    CT_NONE,             // not in a call
    CT_NGC_OWORD_SUB,    // no restartable Python code involved
    CT_NGC_M98_SUB,      // like above; Fanuc-style, pass in params #1..#30
    CT_PYTHON_OWORD_SUB, // restartable Python code may be involved
    CT_REMAP,            // restartable Python code may be involved
};

enum LeadinType {
    PREP_NONE,
    PREP_LINEAR_LEADIN,
    PREP_PLUNGE_LEADIN,
};


#endif // INTERP_ENUMS_HH
