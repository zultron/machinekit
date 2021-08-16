#ifndef GCODE_PREVIEW_DATA_HH
#define GCODE_PREVIEW_DATA_HH

#include "tool_motion_data.hh"
#include "coordinatetransform.hh"

struct EmcPose;

enum PreviewMotionPhase
{
    MOTION_PHASE_NORMAL, // Show both preview motion and extents
    MOTION_PHASE_SETUP, // Show nothing (previously called a "first" move, e.g. rapids after a tool change)
    MOTION_PHASE_RETRACT, // Show preview motion only (for G30 and similar moves, user may want to see the path, but it's not part of the program's true extents)
};

class PreviewMachine
{
    ParseExecMode exec_mode;
    int current_tool_pocket;
    PositionArray machine_pos_uu;
    CANON_PLANE plane;
    double feed_rate;
    PreviewMotionPhase current_motion_phase;

    ToolMotionMap tool_motions;

    void add_motion(ToolMotionKey key, ToolMotion motion);
    std::vector<ToolMotion> &get_motion_set(ToolMotionKey key) {return tool_motions[key];}
    void straight_motion(int line_number, PositionArray const &goal_pos_prog, MotionType t);
    bool updateExtents(const PositionArray & machine_goal_pos_uu);
    void resetExtents();

public:
    std::bitset<64> hide_motion_phase_extents; // Indexed to PreviewMotionPhase
    std::bitset<64> hide_motion_phase_preview;
    ExtentBox extents_tool;
    ExtentBox extents_machine;
    PreviewMachine() = default;
    LocalToMachineTransform transform_uu; //KLUDGE easier to just poke this directly

    PositionArray const &get_current_pos() const {return machine_pos_uu;}
    const PositionArray get_current_pos_local() const;
    const PositionArray get_current_pos_prog() const;
    PositionArray to_preview_coords(PositionArray const &local_pos_prog);
    void set_current_pos(PositionArray const &pos_local_prog);

    // Callbacks for various canon functions
    void straight_feed(int line_number, PositionArray const &goal_pos_prog);
    void straight_probe(int line_number, PositionArray const &goal_pos_prog);
    void straight_traverse(int line_number, PositionArray const &goal_pos_prog);
    void arc_feed(int line_number,
        double first_end,
        double second_end,
        double first_axis,
        double second_axis,
        double rotation,
        double axis_end_point,
        double a_position,
        double b_position,
        double c_position,
        double u_position,
        double v_position,
        double w_position, bool allow_implicit_full_turn);
    void rigid_tap(int line_number, PositionArray const &goal_pos_prog);
    void change_tool(int tool_pocket);
    void set_plane(CANON_PLANE pl) {plane = pl;}
    void set_firstmove();
    void clear_firstmove();
    void tool_offset(const EmcPose & offset);
    void set_feed_rate(double feed_rate_prog);

    // Misc helpers
    bool set_exec_mode(ParseExecMode mode);
    void clear_motions();

    ToolMotionMap const &getToolMotionData() const;

    void begin_retract_phase();
    void begin_feed_phase();
    void begin_setup_phase();

    bool has_valid_extents() const;     
};

// for emcmodule.cc, get tool motion data from the machine object in gcodemodule
ToolMotionMap const &getToolMotionData();

// TODO make dwells native? probably not worth it since they are relatively rare
#endif // GCODE_PREVIEW_DATA_HH
