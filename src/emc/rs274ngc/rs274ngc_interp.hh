//    Copyright 2009-2011, various authors
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef RS274NGC_INTERP_H
#define RS274NGC_INTERP_H
#include "rs274ngc.hh"
#include "interp_internal.hh"
#include "interp_return.hh"
#include <array>
#include <callstacktracker.hh>

using namespace interp_shared;

/**
 * Stores information about a single line of G-code (where it came from, line number, etc.).
 * This is designed for error messages / diagnostics, so it's not
 * necessarily an exhaustive list of state for the G-code line.
 */
struct GCodeContext
{
    GCodeContext() = default;
    GCodeContext(
        std::string line_text,
        int line,
        std::string file_path,
        std::string caller_name,
        std::string py_remap_name);

    std::string line_text;
    int line;
    std::string file_path;
    std::string caller_name;
    std::string py_remap_name;
    int display_line; // The closest line in the base file to this context
};

struct IniFile;
struct GCodeGenerator;

struct GCodeAnnotation
{
    GCodeAnnotation(
        std::string msg,
        int line,
        CanonMessageAlarmLevel severity_lvl=CANON_MSG_MEDIUM) :
        message(msg),
        toplevel_line(line),
        severity(severity_lvl)
    {}
    std::string message;
    int toplevel_line;
    CanonMessageAlarmLevel severity;
};

struct GCodeContextStack
{
    std::vector<GCodeContext> context_lines;
    int location_in_line; // First character of error
    std::string cached_py_remap_name; // Just for internal use
    std::string replacement_local_linetext;
    std::string base_file_name; // For reference only to check if a context level is external
    bool current_context_is_external;
    bool pop();
    void replace_local_linetext(const char *replacement_linetext);
    void reset(const char * base_file_name_in);
    int get_closest_toplevel_caller_line() const;
    void update_display_line();
};

using RestoreCommandList = std::vector<std::string>;

struct CannedCycleConfig
{

    const CANON_PLANE plane;
    PositionArrayIndex axis_0, axis_1, depth_axis;

    double current_cc;

    double aa;
    double aa_increment;
    double bb;
    double bb_increment;
    double cc;
    double clear_cc;
    double i;
    double j;
    double k;
    double old_cc;
    double radius_increment;
    double theta_increment;
    double r;
    int repeat;
    CANON_MOTION_MODE save_mode;
    double save_tolerance;
};

class Interp : public InterpBase {

public:
 mutable CallStackTracker callstack;
 Interp();
 ~Interp();

/* Interface functions to call to tell the interpreter what to do.
   Return values indicate status of execution.
   These functions may change the state of the interpreter. */

// close the currently open NC code file
 int close();
 
// execute a line of NC code
 int execute();

 int execute(const char *command);
 /** Provide an explicit source of the G-code command (for logging purposes) */
 int execute(const char *command, CommandSource cmd_src);

 int execute(const char *command, CommandSource cmd_src, int line_number);

// stop running
 int exit();

// get ready to run
 int init();
 void set_loop_on_main_m99(bool state);

 int collect_expanded_modal_state(ExpandedModalState &initial_state) const;

// load a tool table
 int load_tool_table();

 virtual int synch_work_offsets_from_task(setup_pointer settings) const;

// open a file of NC code
 int open(const char *filename, const char *working_dir);

// after an open() and close() cycle, the interp knows which offsets were used by the
// program so this provides acess to that state.
 std::set<int> get_work_offsets_used();

// read the mdi or the next line of the open NC code file
 int read(const char *command, CommandSource cmd_src);
 int read(const char *command);
 int read();

// reset yourself
 int reset();

// restore interpreter variables from a file
 virtual int restore_parameters(const char *filename);

// save interpreter variables to file
 int save_parameters(
     const char *filename,
     ParameterArray const &parameters);

 // Saves all parameters (including volatiles) to the specified file (typically for preview use)
 int export_all_parameters(const setup_pointer settings,
     const char *filename) const;

// synchronize your internal model with the external world
 int synch();
 int cbBeforeSeekReadTransition(const AutoRunSettings & start_settings, const ExpandedModalState & run_initial_state, ExpandedModalState & seek_state);
 int cbAfterSeekReadTransition(const ExpandedModalState & to_restore,
     const AutoRunSettings & start_settings, const ExpandedModalState & run_initial_state);
 int synch_and_export(setup_pointer settings, const char *export_file_name) const;

/* Interface functions to call to get information from the interpreter.
   If a function has a return value, the return value contains the information.
   If a function returns nothing, information is copied into one of the
   arguments to the function. These functions do not change the state of
   the interpreter. */

// copy active G codes into array [0]..[15]
 void active_g_codes(int *codes);
 void active_g_codes(int *codes, long int line);

// copy active M codes into array [0]..[9]
 void active_m_codes(int *codes);
 void active_m_codes(int *codes, long int line);

// copy active F, S settings into array [0]..[2]
 void active_settings(double *settings);
 void active_settings(double *settings, long int line);

 // Update the state vectors from a state tag
 int active_modes(ModalState & active_state,
     StateTag const &tag);

// copy the text of the error message whose number is error_code into the
// error_text array, but stop at max_size if the text is longer.
 char *error_text(int error_code, char *error_text,
                                size_t max_size);
 std::string error_text(int error_code=INTERP_ERROR);

 void setError(const char *fmt, ...) const __attribute__((format(printf,2,3)));

 void setWarning(const char *fmt, ...) const __attribute__((format(printf,2,3)));

// copy the name of the currently open file into the file_name array,
// but stop at max_size if the name is longer
 char *file_name(char *file_name, size_t max_size);

// return the length of the most recently read line
 size_t line_length() const;

// copy the text of the most recently read line into the line_text array,
// but stop at max_size if the text is longer
 char *line_text(char *line_text, size_t max_size);

// return the current sequence number (how many lines read)
 int sequence_number() const;

// copy the function name from the stack_index'th position of the
// function call stack at the time of the most recent error into
// the function name string, but stop at max_size if the name is longer
 char *stack_name(int stack_index, char *function_name,
                                size_t max_size);

// Get the parameter file name from the ini file.
 int ini_load(const char *filename);

 int line() const { return sequence_number(); }
 const int & call_level() const;
 const int & remap_level() const;

 char *command(char *buf, size_t len) { line_text(buf, len); return buf; }

 char *file(char *buf, size_t len) { file_name(buf, len); return buf; }
 char *working_dir(char *buf, size_t len);

private:
 void copy_tool0_parameters();
public:
 int init_tool_parameters();
 int default_tool_parameters();
 int set_tool_parameters();
 int on_abort(int reason, const char *message);

    void set_loglevel(int level);

    // for now, public - for boost.python access
 int find_named_param(const char *nameBuf, int *status, double *value);
 int store_named_param(setup_pointer settings,const char *nameBuf, double value, int override_readonly = 0);
 int add_named_param(const char *nameBuf, int attr = 0);
 int fetch_ini_param( const char *nameBuf, int *status, double *value);
 int fetch_hal_param( const char *nameBuf, int *status, double *value);

    // common combination of add_named_param and store_named_param
    // int assign_named_param(const char *nameBuf, int attr = 0, double value = 0.0);
    remap_pointer remapping(const char *code);
    remap_pointer remapping(const char letter, int number = -1);
 int find_tool_pocket(setup_pointer settings, int toolno, int *pocket);

    // private:
    //protected:  // for boost wrapper access

/* Function prototypes for all  functions */

 int arc_data_comp_ijk(int move,
         int plane,
         CutterCompDirection side,
         double tool_radius,
         double current_x,
         double current_y,
         double end_x,
         double end_y,
         int ij_absolute,
         double i_number,
         double j_number,
         int p_number,
         double *center_x,
         double *center_y,
         int *turn,
         double radius_tolerance,
         double spiral_abs_tolerance,
         double spiral_rel_tolerance);

 int arc_data_comp_r(int move,
         int plane,
         CutterCompDirection side,
         double tool_radius,
         double current_x,
         double current_y,
         double end_x,
         double end_y,
         double big_radius,
         int p_number,
         double *center_x,
         double *center_y,
         int *turn,
         double radius_tolerance);

 int arc_data_ijk(int move,
         int plane,
         double current_x,
         double current_y,
         double end_x,
         double end_y,
         int ij_absolute,
         double i_number,
         double j_number,
         int p_number,
         double *center_x,
         double *center_y,
         int *turn,
         double radius_tolerance,
         double spiral_abs_tolerance,
         double spiral_rel_tolerance);

 int arc_data_r(int move,
         int plane,
         double current_x,
         double current_y,
         double end_x,
         double end_y,
         double radius,
         int p_number,
         double *center_x,
         double *center_y,
         int *turn,
         double radius_tolerance);
 int check_g_codes(block_pointer block, setup_pointer settings);
 int check_items(block_pointer block, setup_pointer settings);
 int check_m_codes(block_pointer block);
 int check_other_codes(block_pointer block);
 int close_and_downcase(char *line, const char * raw_line, int * length);
 int convert_nurbs(GCodes move, block_pointer block, setup_pointer settings);
 int convert_spline(int move, block_pointer block, setup_pointer settings);
 int comp_get_current(setup_pointer settings, double *x, double *y, double *z);
 int comp_set_current(setup_pointer settings, double x, double y, double z);
 int comp_get_programmed(setup_pointer settings, double *x, double *y, double *z);
 int comp_set_programmed(setup_pointer settings, double x, double y, double z);
 int convert_arc(GCodes move, block_pointer block, setup_pointer settings);
 int convert_arc2(int move, block_pointer block,
                  setup_pointer settings,
                  double *current1, double *current2, double *current3,
                  double end1, double end2, double end3,
                  double AA_end, double BB_end, double CC_end,
                  double u_end, double v_end, double w_end,
                  double offset1, double offset2);

 int convert_arc_comp1(int move, block_pointer block,
                       setup_pointer settings,
                       double end_x, double end_y, double end_z,
                       double offset_x, double offset_y,
                       double AA_end, double BB_end, double CC_end,
                       double u_end, double v_end, double w_end);

 int convert_arc_comp2(int move, block_pointer block,
                       setup_pointer settings,
                       double end_x, double end_y, double end_z,
                       double offset_x, double offset_y,
                       double AA_end, double BB_end, double CC_end,
                       double u_end, double v_end, double w_end);
 char arc_axis1(int plane);
 char arc_axis2(int plane);
 int convert_axis_offsets(int g_code, block_pointer block,
                                setup_pointer settings);
 int convert_param_comment(char *comment, char *expanded, int len);
    int convert_comment(char *comment, bool enqueue = true);
 int convert_control_mode(int g_code, double tolerance, double naivecam_tolerance, setup_pointer settings);
 int convert_adaptive_mode(int g_code, setup_pointer settings);

 int convert_coordinate_system(int g_code, setup_pointer settings);
 int convert_coordinate_system(int g_code, int p_word, setup_pointer settings);

 void send_current_g5x_transform();
 void send_g5x_transform(int origin, RotateShiftTransform const &g5x_transform);

 int convert_cutter_compensation(int g_code, block_pointer block,
                                       setup_pointer settings);
 int convert_cutter_compensation_off(setup_pointer settings);
 int convert_cutter_compensation_on(CutterCompDirection side, block_pointer block,
                                          setup_pointer settings);
 int convert_cycle(GCodes motion, block_pointer block,
                         setup_pointer settings);
 int convert_cycle_for_plane(int motion,
                            block_pointer block,
                             setup_pointer settings);
 int convert_cycle_g81(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double clear_z, double bottom_z);
 int convert_cycle_g82(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double clear_z, double bottom_z, double dwell);
 int convert_cycle_g73(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double r, double clear_z, double bottom_z,
                             double delta);
 int convert_cycle_g83(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double r, double clear_z, double bottom_z,
                             double delta);
 int convert_cycle_g74_g84(PositionArray const &cycle_end, CANON_PLANE plane, double planar_pos_1, double planar_pos_2,
                             double clear_height, double bottom_depth,
                             CANON_DIRECTION direction,
                             CANON_SPEED_FEED_MODE mode, int motion, double dwell);
 int convert_cycle_g85(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double r, double clear_z, double bottom_z);
 int convert_cycle_g86(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double clear_z, double bottom_z, double dwell,
                             CANON_DIRECTION direction);
 int convert_cycle_g87(PositionArray const &cycle_end, CANON_PLANE plane, double x, double offset_x,
                             double y, double offset_y, double r,
                             double clear_z, double middle_z, double bottom_z,
                             CANON_DIRECTION direction);
 int convert_cycle_g88(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double bottom_z, double dwell,
                             CANON_DIRECTION direction);
 int convert_cycle_g89(PositionArray const &cycle_end, CANON_PLANE plane, double x, double y,
                             double clear_z, double bottom_z, double dwell);
 int convert_distance_mode(int g_code, setup_pointer settings);
 int convert_ijk_distance_mode(int g_code, setup_pointer settings);
 int convert_lathe_diameter_mode(int g_code, block_pointer block, setup_pointer settings);
 int convert_dwell(setup_pointer settings, block_pointer block, double time);
 int convert_feed_mode(int g_code, setup_pointer settings, block_pointer block);
 int convert_feed_rate(block_pointer block, setup_pointer settings);
    int convert_g(block_pointer block, setup_pointer settings);
 int convert_home(int move, block_pointer block,
                        setup_pointer settings);
 int convert_savehome(int move, block_pointer block,
                        setup_pointer settings);
 int convert_length_units(int g_code, setup_pointer settings);
    int convert_m(block_pointer block, setup_pointer settings);
 int convert_modal_0(GCodes code, block_pointer block,
                           setup_pointer settings);
 int convert_motion(GCodes motion, block_pointer block,
                          setup_pointer settings);
 int convert_probe(block_pointer block, GCodes g_code, setup_pointer settings);
 int convert_retract_mode(int g_code, setup_pointer settings);
 int convert_part_count(block_pointer block, setup_pointer settings);
 int convert_setup_work(block_pointer block, setup_pointer settings);
 int convert_setup_tool(block_pointer block, setup_pointer settings);
 int convert_set_plane(int g_code, setup_pointer settings);
 int convert_speed(block_pointer block, setup_pointer settings);
 int convert_spindle_mode(block_pointer block, setup_pointer settings);
 int convert_stop(block_pointer block, setup_pointer settings);
 int increment_part_counter(setup_pointer settings, size_t counter_id);
 int update_part_counter(setup_pointer settings, size_t counter_id, int part_count);
 int convert_straight(GCodes move, block_pointer block,
                            setup_pointer settings);
 int convert_straight_comp1(int move, block_pointer block,
                            setup_pointer settings,
                            double px, double py, double end_z,
                            double AA_end, double BB_end, double CC_end,
                            double u_end, double v_end, double w_end);
 int convert_straight_comp2(int move, block_pointer block,
                            setup_pointer settings,
                            double px, double py, double end_z,
                            double AA_end, double BB_end, double CC_end,
                            double u_end, double v_end, double w_end);
 int convert_threading_cycle(block_pointer block, setup_pointer settings,
                             double end_x, double end_y, double end_z);
 int convert_tool_change(setup_pointer settings);
 int convert_lathe_tool_change(block_pointer block, setup_pointer settings);
 int convert_tool_length_offset(int g_code, block_pointer block,
                                      setup_pointer settings);
 int convert_tool_select(block_pointer block, setup_pointer settings);
 int update_tag(StateTag &tag);
 int send_tag_io_msg();
 PositionArray get_cycle_position(const PositionArray & cycle_end, CANON_PLANE plane, double plane_x, double plane_y, double height);
 int cycle_feed(const PositionArray & cycle_end, CANON_PLANE plane, double end1,
                double end2, double end3);
 int cycle_traverse(const PositionArray & cycle_end, CANON_PLANE plane, double plane_x, double plane_y, double height);
 int enhance_block(block_pointer block, setup_pointer settings);
protected:
 int execute_impl(const char *command, CommandSource cmd_src);
public:
 int execute_binary(double *left, int operation, double *right);
 int execute_binary1(double *left, int operation, double *right);
 int execute_binary2(double *left, int operation, double *right);
 virtual bool need_step_increment(const setup_struct *settings) const;
 int execute_block(block_pointer block, setup_pointer settings);
 int execute_unary(double *double_ptr, int operation);
 double find_arc_length(double x1, double y1, double z1,
                              double center_x, double center_y, int turn,
                              double x2, double y2, double z2);
 DISTANCE_MODE get_distance_mode(block_pointer block, setup_pointer settings) const;

 int clearPersistentG92Parameters(setup_pointer settings);
 ShiftTransform getG92ActiveOffset(setup_pointer settings);
 int storeG92OffsetValues(ShiftTransform const &g92_offset, setup_pointer settings);
 int storeG92Applied(bool active, setup_pointer settings);

 int find_tool_offset(setup_pointer settings, int pocket, ShiftTransform & tool_offset);
 int store_tool_offset(setup_pointer settings, ShiftTransform const &tool_offset, int pocket);
 int canon_use_tool_offset(ShiftTransform const &tool_offset);
 InterpReturn find_work_offset_from_parameters(setup_pointer settings, int origin, RotateShiftTransform & work_offset_user);
 int store_work_offset_to_parameters(setup_pointer settings, RotateShiftTransform const &work_offset, int origin) const;
 int find_work_offset_prog(int origin, RotateShiftTransform & work_offset_user);
 int store_work_offset_from_prog(setup_pointer settings, RotateShiftTransform const &work_offset_prog, int origin) const;
 int store_work_offset(setup_pointer settings, RotateShiftTransform const &work_offset_user, int origin) const;
 int find_current_in_system(setup_pointer s, int system, PositionArray &out);
 int find_current_in_system_without_tlo(setup_pointer s, int system, PositionArray & out);
 int find_ends(block_pointer block, setup_pointer settings,
               double *px, double *py, double *pz,
               double *AA_p, double *BB_p, double *CC_p,
               double *u_p, double *v_p, double *w_p);
 int find_ends(block_pointer block, setup_pointer settings,
               PositionArray &cmd_local_pos);
 int find_relative(double x1, double y1, double z1,
                   double AA_1, double BB_1, double CC_1,
                   double u_1, double v_1, double w_1,
                   double *x2, double *y2, double *z2,
                   double *AA_2, double *BB_2, double *CC_2,
                   double *u_2, double *v_2, double *w_2,
                   setup_pointer settings);
 int find_relative(setup_pointer s,
               PositionArray const &command_abs,
               PositionArray &cmd_local);
 double find_straight_length(double x2, double y2, double z2,
                             double AA_2, double BB_2, double CC_2,
                             double u_w, double v_2, double w_2,
                             double x1, double y1, double z1,
                             double AA_1, double BB_1, double CC_1,
                             double u_1, double v_1, double w_1);
 double find_turn(double x1, double y1, double center_x,
                        double center_y, int turn, double x2, double y2);
 int init_block(block_pointer block);
 int inverse_time_rate_arc(double x1, double y1, double z1,
                                 double cx, double cy, int turn, double x2,
                                 double y2, double z2, block_pointer block,
                                 setup_pointer settings);
 int inverse_time_rate_straight(double end_x, double end_y, double end_z,
                                double AA_end, double BB_end, double CC_end,
                                double u_end, double v_end, double w_end,
                                block_pointer block,
                                setup_pointer settings);
 int move_endpoint_and_flush(setup_pointer, double, double);
 int parse_line(
     char *line,
     int line_length,
     block_pointer block,
     setup_pointer settings);
 int precedence(int an_operator);
protected:
 int read_impl(const char *command);
public:
 int read_a(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_atan(char *line, int *counter, double *double_ptr,
                     ParameterArray &parameters);
 int read_atsign(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_b(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_c(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_carat(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_comment(char *line, int *counter, block_pointer block,
                        ParameterArray &parameters);
 int read_semicolon(char *line, int *counter, block_pointer block,
                        ParameterArray &parameters);
 int read_d(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_e(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_f(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_g(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_h(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_i(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_integer_unsigned(char *line, int *counter, int *integer_ptr);
 int read_integer_value(char *line, int *counter, int *integer_ptr,
                              ParameterArray &parameters);
 int read_items(block_pointer block, char *line, int line_length, ParameterArray &parameters);
 int read_j(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_k(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_l(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_n_number(char *line, int *counter, block_pointer block);
 int read_m(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_o(char *line, int line_length, int *counter, block_pointer block,
                  ParameterArray &parameters);


 int read_one_item(char *line, int *counter, block_pointer block,
                   ParameterArray &parameters);
 int read_operation(char *line, int *counter, int *operation);
 int read_operation_unary(char *line, int *counter, int *operation);
 int read_p(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);



 int lookup_named_param(const char *nameBuf, double index, double *value);
    int init_readonly_param(const char *nameBuf, double value, int attr);
    int free_named_parameters(context_pointer frame);
 int save_settings(setup_pointer settings) const;
 int restore_settings(setup_pointer settings, int from_level, int caller_m_code);
 int restore_settings_impl(ModalState & current, ModalState const &saved, int caller_m_code);

 virtual int write_modal_state(block_pointer const block, setup_pointer settings);
 int refresh_modal_state(setup_pointer settings);
 int collect_modal_state(block_pointer const block, const setup_struct * settings, ModalState &out) const;
 int execute_gcode_sequence(const RestoreCommandList & cmd_list, int caller_m_code, const char * caller_name);

 int restore_from_tag(StateTag const &tag);

 int create_full_restore_sequence(const ExpandedModalState & to_restore,
     RestoreCommandList &restore_cmd_list,
     const InterpPreamble &preamble,
     const AutoRunSettings & start_settings, const ExpandedModalState & run_initial_state);

 int read_name(char *line, int *counter, char *nameBuf);
 int read_named_parameter(char *line, int *counter, double *double_ptr,
                          ParameterArray &parameters, bool check_exists);
 int read_parameter(char *line, int *counter, double *double_ptr,
                          ParameterArray &parameters, bool check_exists);
 int read_parameter_setting(char *line, int *counter,
                                  block_pointer block, ParameterArray &parameters);
 int read_bracketed_parameter(char *line, int *counter, double *double_ptr,
                          ParameterArray &parameters, bool check_exists);
 int read_named_parameter_setting(char *line, int *counter,
                                  char **param, ParameterArray &parameters);
 int read_q(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_r(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_real_expression(char *line, int *counter,
                                double *hold2, ParameterArray &parameters);
 int read_empty_expression(char *line, int *counter, const char * caller_name);
 int read_real_number(const char * line, int *counter, double *double_ptr);
 int read_real_value(char *line, int *counter, double *double_ptr,
                           ParameterArray &parameters);
 int read_s(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_t(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_text(const char *command, FILE * inport, char *raw_line,
                     char *line, int *length);
 int read_unary(char *line, int *counter, double *double_ptr,
                      ParameterArray &parameters);
 int read_u(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_v(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_w(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_x(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_y(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_z(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int read_missing_axis(char *line, int *counter, block_pointer block,
                  ParameterArray &parameters);
 int refresh_actual_position(setup_pointer settings);
 void rotate(double *x, double *y, double t);
 int set_probe_data(setup_pointer settings);
 static int formatGCode(GCodes g, std::string &cmd);
 static int lookup_work_offset_gcode_from_origin(int origin);
 static std::string lookup_work_offset_gcode_string_from_origin(int origin);
 int write_g_codes(block_pointer block, const setup_struct * settings, ModalState &state) const;
 int write_m_codes(block_pointer block, const setup_struct * settings, ModalState &state) const;
 int write_settings(const setup_struct * settings, ModalState &state) const;
 virtual int write_and_update_state_tag(block_pointer block=nullptr, int line_number=0);
 int unwrap_rotary(double *, double, double, double, char);
 bool isreadonly(int index);

  // O_word stuff

 int findFile( // ARGUMENTS
		     char *direct,  // the directory to start looking in
		     char *target,  // the name of the file to find
		     char *foundFileDirect); // where to store the result

 int control_save_offset(    /* ARGUMENTS                   */
			 // int line,                  /* (o-word) line number        */
  block_pointer block,       /* pointer to a block of RS274/NGC instructions */
  setup_pointer settings);   /* pointer to machine settings */

 int control_save_offset(
  block_pointer block,
  const char *o_name,        /* o_name key */
  setup_pointer settings);

 int control_find_oword(     /* ARGUMENTS                   */
  const char *o_name,        /* o-word name                 */
  setup_pointer settings,    /* pointer to machine settings */
  offset_pointer *ppo);

 int control_find_oword(     /* ARGUMENTS                   */
  block_pointer block,       /* block pointer to get (o-word) name        */
  setup_pointer settings,    /* pointer to machine settings */
  offset_pointer *ppo);
//  int *o_index);             /* the index of o-word (returned) */

 int control_back_to(        /* ARGUMENTS                   */
  block_pointer block, // pointer to block
  setup_pointer settings);   /* pointer to machine settings */

 // establish a new subroutine context
 int enter_context(setup_pointer settings, block_pointer block);
 // leave current subroutine context
 int leave_context(setup_pointer settings, bool restore = true);

    //int call_fsm(setup_pointer settings, int event);
    //int execute_pycall(setup_pointer settings, const char *name, int call_phase);
 int execute_call(setup_pointer settings, context_pointer current_frame, int call_type);  
 int execute_return(setup_pointer settings,  context_pointer current_frame, int call_type);  
 void loop_to_beginning(setup_pointer settings);
    //int execute_remap(setup_pointer settings, int call_phase);   // remap call state machine
    int handler_returned( setup_pointer settings, 
			  context_pointer active_frame, const char *name, bool osub);
int read_inputs(setup_pointer settings);

 int convert_control_functions( /* ARGUMENTS           */
  block_pointer block,       /* pointer to a block of RS274/NGC instructions */
  setup_pointer settings);   /* pointer to machine settings */


 // parse a REMAP= descriptor from the ini file
 int parse_remap(const char *inistring, int lineno);

 // step through parsed block and collect remapped items in
 // block.remappings set
    int find_remappings(block_pointer block, setup_pointer settings);

 // establish a new remapping context
 int enter_remap(void);
 // leave current remapping context
 int leave_remap(void);

 // callback when remapping handler done
 int remap_finished( int status);

 int report_error(setup_pointer settings,int status,const char *text);

 //  add named params/param dict if argspec given
 // present optional words to the subroutine's local variables and Py dict
    int add_parameters(setup_pointer settings, block_pointer cblock,
		       char *posarglist);

 int init_named_parameters();
    int init_python_predef_parameter(const char *name);

    bool has_user_mcode(setup_pointer settings,block_pointer block);

    // initialize the m_remappable and g_remappable sets
    // which define which M and G codes could be subject to a remap
    // this is more sane and comprehensible than the M_REMAPPABLE()
    // and G_REMAPPABLE() macros (and faster too)
 int init_remap_sets();

#define M_BUILTIN(m) (_ems[m] != -1)
#define G_BUILTIN(g) (_gees[g] != -1)

    bool M_REMAPPABLE(int m)
    {
        return (m >= 0) && (m < 1000) && _setup.m_remappable[m];
    }
    // range for user-remapped G-codes
    bool G_REMAPPABLE(int g)
    {
        return (g >= 0) && (g < 1000) &&_setup.g_remappable[g];
    }

    bool IS_USER_GCODE(int x) {
        return G_REMAPPABLE(x) && _setup.g_remapped[x];
    }

    bool IS_USER_MCODE(
        block_pointer block,
        setup_pointer settings,
        int mgroup)
    {
        return M_REMAPPABLE(block->m_modes[mgroup]) && (block->m_modes[mgroup] > -1) && (settings->m_remapped[block->m_modes[mgroup]]);
    }
    
    bool remap_in_progress(const char *code);
    int convert_remapped_code(block_pointer block,
			       setup_pointer settings,
			      int phase,
			      char letter,
			      int number = -1);

    bool is_pycallable(setup_pointer settings, const char *module, const char *funcname);

#define OWORD_MODULE "oword"
#define REMAP_MODULE "remap"
#define NAMEDPARAMS_MODULE "namedparams"
    // describes intented use, and hence parameter and return value
    // interpretation
    enum py_calltype { PY_OWORDCALL,
		       PY_FINISH_OWORDCALL,
		       PY_PROLOG,
		       PY_FINISH_PROLOG,
		       PY_BODY,
		       PY_FINISH_BODY,
		       PY_EPILOG,
		       PY_FINISH_EPILOG,
		       PY_INTERNAL,
		       PY_EXECUTE,
		       PY_PLUGIN_CALL
    };
    int pycall(setup_pointer settings,
	       context_pointer frame,
	       const char *module,
	       const char *funcname,
	       int calltype);
    int py_execute(const char *cmd, bool as_file = false); // for (py, ....) comments
    int py_reload();
    FILE *find_ngc_file(setup_pointer settings,const char *basename, char *foundhere = nullptr);

    const char *getSavedError() const;
    // set error message text without going through printf format interpretation
    int setSavedError(const char *msg);
    int clearSavedError();
    int clearWarning();

    int unwind_call(int status, const char *file, int line, const char *function, CommandSource cmd_src);
    std::string get_command_caller(CommandSource cmd_src);
    int add_error_call_context(CommandSource cmd_src);

    int get_toplevel_line_number() const;

    int convert_straight_indexer(int, block*, setup*);
    int issue_straight_index(int, double, int, setup*);

    void doLog(unsigned int flags, const char *file, int line,
               const char *fmt, ...) const __attribute__((format(printf,5,6)));

    const char *interp_status(int status);

    //technically this violates encapsulation rules but is needed for
    // the Python introspection module

    FILE *log_file;

    /* Internal arrays */
    static const int _gees[];
    static const int _ems[];
    static const int _required_parameters[];
    static const int _readonly_parameters[];
    static const int _n_readonly_parameters;
    std::array<read_function_pointer, 256> _readers;
    static const std::array<read_function_pointer, 256> default_readers;

    setup _setup;
    boost::python::object *pythis;  // boost::cref to 'this'

    CommandSource last_cmd_src;

    GCodeContextStack gcode_context_stack;
    mutable std::deque<GCodeAnnotation> gcode_annotations;

    double getProgLenUnits() const;
    static double getProgAngUnits();
    double TO_PROG_LEN(double mm) const;
    double FROM_PROG_LEN(double) const;
    double TO_PROG_ANG(double) const;
    double FROM_PROG_ANG(double) const;

    double USER_TO_PROGRAM_LEN(double u) const;
    double PROGRAM_TO_USER_LEN(double p) const;
    double USER_TO_PROGRAM_ANG(double u) const;
    double PROGRAM_TO_USER_ANG(double p) const;
    UnitsTransform convertsUserToProgram() const;
    UnitsTransform convertsProgramToUser() const;

    int convert_g10(block_pointer block, setup_pointer settings);
    InterpReturn check_g10(block_pointer block);

    void add_context_line(
        std::ostream &context_msg,
        const char * caller_filename,
        const char * subName,
        int callee_line,
        const char * callee_linetext
        );

    void push_gcode_context(setup_pointer settings, block_pointer block);
    void push_local_gcode_context(setup_pointer settings);
    void update_gcode_context(const setup_pointer settings);
    void update_gcode_context_py_remap_name(const remap_pointer remap);

    bool hasFeature(Features f) const;
    int find_g43_tool_offset(int h_number, bool h_flag, setup_pointer settings, ShiftTransform & new_tool_offset);

    static AxisMask axis_mask_from_string(const std::string & plunge_axes, AxisMask default_axes=0);
    
    int load_interp_preamble(IniFile & inifile);

    ShiftTransform getG92ParameterValues(setup_pointer settings);

    int gen_toolOffset(GCodeGenerator & generator, GCodes tool_offset_mode, int tool_offset_number);

    FILE * try_load_subroutine_from_path(const char * basename, const char * sub_directory, char * file_path);

    int synch_external_state(setup_pointer settings) const;

    virtual bool is_preview() const {return false;}

    int validate_motion_mode(int motion_to_be);

    int get_absolute_polar_point(block_pointer block, const PositionArray & current, CANON_PLANE plane, PositionArray & cmd_local_pos) const;
    int get_absolute_polar(block_pointer block, double p0_current, double p1_current, double &p0, double &p1) const;
    int get_relative_polar_point(block_pointer block, CANON_PLANE plane, PositionArray &cmd_local_pos) const;
    int add_relative_polar(block_pointer block, double &p0, double &p1) const;

    virtual int check_leadin_feedmode(const ModalState & initial, const ModalState & to_restore, int start_line, double leadin_feedrate) const;
    
protected:
    bool error_has_context;

    virtual int check_startline_leadin_feedrate(double & feed_rate_uu, const char * move_type);

    StateTag make_state_tag(setup_struct const &s, block_pointer block, int toplevel_line=0) const;
};


bool toolOffsetActive(setup_struct const *s);
std::tuple<PositionArrayIndex, PositionArrayIndex, PositionArrayIndex> plane_indices(CANON_PLANE plane);

#define STATE_RESTORE_SUBROUTINE_NAME "restore_start_line_state"


/**
 * Return the first position after the "needle" string if the haystack string starts with it, else 0.
 * @warning used only with compile-time strings (typically as static const char arrays).
 * @pre needle should be a non-empty string (this function will only find a non-trivial match) that is null-terminated
 */
template<size_t N>
size_t startswith(char *haystack, const char (&needle)[N]) {
    return !strncmp(haystack, needle, N-1) ? N-1 : 0;
}

template<size_t N>
size_t startswith(char *, const char (&)[1]) {
    return false;
}
template<size_t N>
size_t startswith(char *, const char (&)[2]) {
    return false;
}


#endif
