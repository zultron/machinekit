/*
 * Copyright (C) 2013 Jeff Epler <jepler@unpythonic.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdlib.h>
#include <modal_state.hh>

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
    ACTIVE_SETTINGS
} ActiveSettingIndex;

class InterpBase {
public:
    virtual ~InterpBase();
    virtual char *error_text(int errcode, char *buf, size_t buflen) = 0;
    virtual char *warning_text(char *buf, size_t buflen) = 0;
    virtual char *line_text(char *buf, size_t buflen) = 0;
    virtual char *file_name(char *buf, size_t buflen) = 0;
    virtual char *stack_name(int index, char *buf, size_t buflen) = 0;
    virtual size_t line_length() = 0;
    virtual int sequence_number() = 0;
    virtual int ini_load(const char *inifile) = 0;
    virtual int init() = 0;
    virtual int execute() = 0;
    virtual int execute(const char *line) = 0;
    virtual int execute(const char *line, int line_number) = 0;
    virtual int synch() = 0;
    virtual int exit() = 0;
    virtual int open(const char *filename) = 0;
    virtual int read() = 0;
    virtual int read(const char *line) = 0;
    virtual int close() = 0;
    virtual int reset() = 0;
    virtual int line() = 0;
    virtual int call_level() = 0;
    virtual char *command(char *buf, size_t buflen) = 0;
    virtual char *file(char *buf, size_t buflen) = 0;
    virtual int on_abort(int reason, const char *message) = 0;
    virtual void active_g_codes(int active_gcodes[ACTIVE_G_CODES]) = 0;
    virtual void active_m_codes(int active_mcodes[ACTIVE_M_CODES]) = 0;
    virtual void active_settings(double active_settings[ACTIVE_SETTINGS]) = 0;
    virtual int active_modes(int g_codes[ACTIVE_G_CODES],
            int m_codes[ACTIVE_M_CODES],
            double settings[ACTIVE_SETTINGS],
            StateTag const &tag) = 0;
    virtual int restore_from_tag(StateTag const &tag) = 0;
    virtual void set_loglevel(int level) = 0;
    virtual void set_loop_on_main_m99(bool state) = 0;
};

InterpBase *interp_from_shlib(const char *shlib);
extern "C" InterpBase *makeInterp();
