/*
 * Copyright: 2013-4
 * Author: Jeff Epler <jepler@unpythonic.net>
 * Author: Michael Haberler <git@mah.priv.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string.h>
#include "rs274ngc_interp.hh"
#include <boost/python/object.hpp>
#include <algorithm>
#include "work_offset_structs.hh"

#pragma GCC diagnostic error "-Wmissing-field-initializers"

setup_struct::setup_struct() :
    current(),
    transform(),
    work_offsets_user(),
    // Only 2 work offsets should be able to be updated between synchs
    // (one via G5x, one via G10 L2 P###). Allowing a 3rd is a harmless safety factor.
    work_offsets_to_synch(3),
    modal_state(),

    arc_not_allowed(0),

    blocks(),
    remap_level(0),
    blocktext{},
    control_mode(CANON_EXACT_STOP),
    cutter_comp(),
    current_pocket(0),

    cycle(),
    distance_mode(MODE_ABSOLUTE),

    ijk_distance_mode(MODE_ABSOLUTE),
    feed_mode(0),
    feed_override(0),
    feed_rate (0.0),
    filename{},
    file_pointer(NULL),
    flood(0),
    length_units(CANON_UNITS_INCHES),
    line_length(0),
    linetext{},
    mist(0),
    motion_mode(G_INVALID),
    start_line_motion_mode(G_INVALID),
    origin_index(0),

    parameters{0},
    new_parameters(),
    new_named_parameters(),
    percent_flag(0),
    plane(CANON_PLANE_XY),
    wait_flags(0),
    input_index(0),
    input_digital(0),
    program_x (0.0),
    program_y (0.0),
    program_z (0.0),
    random_toolchanger(0),
    selected_pocket(0),
    selected_tool(0),
    tool_offset_mode(G_49),
    sequence_number(0),
    stepping_id(0),
    speed (0.0),
    spindle_mode(CONSTANT_RPM),
    speed_feed_mode(CANON_INDEPENDENT),
    speed_override(0),
    spindle_turning(CANON_STOPPED),
    pockets_max(0),
    tool_table(),
    traverse_rate (0.0),
    orient_offset (0.0),

    defining_sub(0),
    sub_name(NULL),
    doing_continue(0),
    doing_break(0),
    executed_if(0),

    skipping_o(NULL),
    skipping_to_sub(NULL),
    skipping_start(0),
    test_value (0.0),
    return_value (0.0),
    value_returned(0),
    call_level(0),
    sub_context(),
    call_state(0),
    allow_adaptive_feed(0),
    allow_feed_hold(0),
    loggingLevel(0),
    debugmask(0),
    log_file{},
    program_prefix{},
    subroutines{},
    use_lazy_close(0),
    lazy_closing(0),
    wizard_root{},
    tool_change_at_g30(0),
    tool_change_quill_up(0),
    tool_change_with_spindle_on(0),
    a_axis_wrapped(0),
    b_axis_wrapped(0),
    c_axis_wrapped(0),

    a_indexer(0),
    b_indexer(0),
    c_indexer(0),

    lathe_diameter_mode(0),
    mdi_interrupt(0),
    feature_set(0),
    single_block_local_sub(false),
    single_block_external_sub(false),
    persistent_g92_offset(false),
    disable_fanuc_style_sub(false),
    loop_on_main_m99(false),
    pythis(),
    on_abort_command(nullptr),
    init_once(1),
    m_remappable(),
    g_remappable(),
    work_offsets_used(),
    tool_offsets_changed()
{
  blocks.fill(block());
  parameters.fill(0);
  tool_table.fill(CANON_TOOL_TABLE());
  sub_context.fill(context{});

  // Prevent unnecessary reallocations
  new_parameters.reserve(MAX_NAMED_PARAMETERS);
  new_named_parameters.reserve(MAX_NAMED_PARAMETERS);
}
    // not sure about sub_context:
    // a plain array of a struct with non-POD data?
int setup_struct::get_toplevel_line_number() const
{
    bool in_remap = (remap_level > 0);
    bool in_subroutine = (call_level > 0 && remap_level == 0);
    bool external_sub = strcmp(filename, sub_context[0].filename);
    /*
     * The line number is for cosmetic labelling only. Based on the interpreter
     * settings from the INI file, we can substitute the controlling block's
     * line number for the actual local line number. One use of this is with
     * external subroutines. If the UI doesn't have the means to display the
     * text of these subroutines, then the line numbers will be wrong if
     * overlaid on the parent file.
    */
    if (in_remap) {
        return blocks[1].saved_line_number;
    }
    else if (in_subroutine && external_sub && !single_block_external_sub) {
        return sub_context[0].sequence_number;
    }
    else if (in_subroutine && !external_sub && !single_block_local_sub) {
        return sub_context[0].sequence_number;
    }
    else {
        return sequence_number;
    }
}

void setup_struct::clear_queue_buster_flags()
{
    wait_flags.reset();
}

block & setup_struct::getControllingBlock()
{
    return blocks[remap_level];
}

block & setup_struct::getExecutingBlock()
{
    return blocks[0];
}

void setup_struct::setExecutingBlock(const block & b)
{
    blocks[0] = b;
}

block &setup_struct::updateControllingBlock()
{
    getControllingBlock() = getExecutingBlock();
    return getControllingBlock();
}

int setup_struct::getCurrentToolNumber() const
{
    return tool_table[current_pocket].toolno;
}

PositionArray setup_struct::posWithDiameterMode(const PositionArray & p_native) const
{
    auto p_out = p_native;
    if (lathe_diameter_mode) {
        p_out[X] *= 2.0;
    }
    return p_out;
}

PositionArray setup_struct::posWithDiameterMode() const
{
    return posWithDiameterMode(current);
}

PositionArray setup_struct::posStripDiameterMode(const PositionArray & p_diam) const
{
    auto p_out = p_diam;
    if (lathe_diameter_mode) {
        p_out[X] /= 2.0;
    }
    return p_out;
}


bool setup_struct::checkFeatureFlag(Features f) const
{
    return has_feature(feature_set, f);
}

setup_struct::~setup_struct() {
    assert(!pythis || Py_IsInitialized());
    if(pythis) delete pythis;
}

block_struct::block_struct ()
    : a_number(0),
      b_number(0),
      c_number(0),
      comment{},
      d_number_float(0),
      e_number(0),
      f_number(0),
      g_modes(),
      h_number(0),
      i_number(0),
      j_number(0),
      k_number(0),
      l_number(0),
      line_number(0),
      saved_line_number(0),
      n_number(0),
      motion_to_be(G_INVALID),
      m_count(0),
      m_modes(),
      p_number(0),
      q_number(0),
      r_number(0),
      s_number(0),
      t_number(0),
      u_number(0),
      v_number(0),
      w_number(0),
      x_number(0),
      y_number(0),
      z_number(0),
      radius(0.0),
      theta(0.0),
// Flags
      a_flag(false),
      b_flag(false),
      c_flag(false),
      d_flag(false),
      e_flag(false),
      f_flag(false),
      h_flag(false),
      i_flag(false),
      j_flag(false),
      k_flag(false),
      l_flag(false),
      p_flag(false),
      q_flag(false),
      r_flag(false),
      s_flag(false),
      t_flag(false),
      u_flag(false),
      v_flag(false),
      w_flag(false),
      x_flag(false),
      y_flag(false),
      z_flag(false),
      radius_flag(false),
      theta_flag(false),

      offset(0), o_type(O_none), call_type(CT_NONE), o_name(nullptr),
      params{}, param_cnt(0), breadcrumbs(),
      executing_remap(NULL), remappings(), phase(0), builtin_used(0) {
    g_modes.fill(G_INVALID);
    m_modes.fill(-1);
}

const char * GetFeatureStr(Features f) {
    switch (f) {
    case FEATURE_RETAIN_G43:
        return "FEATURE_RETAIN_G43";
    case FEATURE_OWORD_N_ARGS:
        return "FEATURE_OWORD_N_ARGS";
    case FEATURE_INI_VARS:
        return "FEATURE_INI_VARS";
    case FEATURE_HAL_PIN_VARS:
        return "FEATURE_HAL_PIN_VARS";
    case FEATURE_NO_DOWNCASE_OWORD:
        return "FEATURE_NO_DOWNCASE_OWORD";
    case FEATURE_OWORD_WARNONLY:
        return "FEATURE_OWORD_WARNONLY";
    case FEATURE_LATHE_TOOLCHANGE:
        return "FEATURE_LATHE_TOOLCHANGE";
    case FEATURE_EXTENDED_ERROR_MESSAGES:
        return "FEATURE_EXTENDED_ERROR_MESSAGES";
    case MAX_FEATURES:
        return "";
    }
    return "";
}

void setup_struct::setXYRotationDeg(double rot_deg)
{
    transform.g5xOffset.setXYRotationDeg(rot_deg);
}
