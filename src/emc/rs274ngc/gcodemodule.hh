#ifndef GCODE_PREVIEW_DATA_HH
#define GCODE_PREVIEW_DATA_HH

#include "tool_motion_data.hh"

class PreviewMachine
{
    ParseExecMode exec_mode;
    int current_tool_pocket;
    PositionArray machine_pos_uu;
    CANON_PLANE plane;
    double feed_rate;

    ToolMotionMap tool_motions;

    void add_motion(ToolMotionKey key, ToolMotion motion);
    std::vector<ToolMotion> &get_motion_set(ToolMotionKey key) {return tool_motions[key];}
    void straight_motion(int line_number, PositionArray const &goal_pos_prog, MotionType t);
    void updateExtents(ToolMotion const &motion);
    void resetExtents();

public:
    ExtentBox extents;
    ExtentBox extents_no_tool;
    PreviewMachine() = default;
    LocalToMachineTransform transform_uu; //KLUDGE easier to just poke this directly
    void straight_feed(int line_number, PositionArray const &goal_pos_prog);
    void straight_probe(int line_number, PositionArray const &goal_pos_prog);
    void straight_traverse(int line_number, PositionArray const &goal_pos_prog);
    void arc_feed(
        int line_number,
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
        double w_position);
    void change_tool(int tool_pocket);
    void set_plane(CANON_PLANE pl) {plane = pl;}
    void set_firstmove();
    void clear_firstmove();
    void tool_offset(const EmcPose & offset);

    // Misc helpers
    bool set_exec_mode(ParseExecMode mode);
    void clear_motions();

    ToolMotionMap const &getToolMotionData() const;
};

// for emcmodule.cc, get tool motion data from the machine object in gcodemodule
ToolMotionMap const &getToolMotionData();

// TODO make dwells native? probably not worth it since they are relatively rare
#endif // GCODE_PREVIEW_DATA_HH
