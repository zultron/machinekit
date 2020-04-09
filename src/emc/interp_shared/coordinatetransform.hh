#ifndef COORDINATETRANSFORM_HH
#define COORDINATETRANSFORM_HH

#include <positionarray.hh>

struct TransformBase
{
    // Includes absolute offsets
    virtual void apply_fwd(PositionArray &p) const = 0;
    virtual void apply_inv(PositionArray &p) const = 0;

    virtual PositionArray forward(PositionArray const &p) const;
    virtual PositionArray inverse(PositionArray const &p) const;

    virtual PositionArray operator()(PositionArray const &p) const;

    virtual ~TransformBase() = default;
};

struct ShiftTransform : public TransformBase
{
    PositionArray tf;

    ShiftTransform();
    explicit ShiftTransform(PositionArray const &pos);

    PositionArray::value_type &operator[](PositionArrayIndex idx);
    PositionArray::value_type const &operator[](PositionArrayIndex idx) const;

    // All 9 axes
    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;
};

struct RotateShiftTransform : public TransformBase
{
    ShiftTransform shift;
    double xy_rotation_rad;

    RotateShiftTransform() = default;
    RotateShiftTransform(PositionArray const &pos,
                         double xy_rot_deg);

    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

    void apply_fwd_relative(PositionArray &p) const;
    void apply_inv_relative(PositionArray &p) const;

    double getXYRotationDeg() const;

    void setXYRotationDeg(double xy_rot_deg);

    void rotate_xy(double &x, double &y) const;
    void unrotate_xy(double &x, double &y) const;
};

struct UnitsTransform : public TransformBase
{
    double linear_fwd;
    double angular_fwd;

    static void scale(PositionArray &p, double linear_fwd, double angular_fwd);

    UnitsTransform(double linear_in, double angular_in);

    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

    ShiftTransform forward_tf(ShiftTransform const &p) const;
    ShiftTransform inverse_tf(ShiftTransform const &p) const;
    RotateShiftTransform forward_tf(RotateShiftTransform const &p) const;
    RotateShiftTransform inverse_tf(RotateShiftTransform const &p) const;

    UnitsTransform &invert_scale();
};

/**
 * Contains transforms required to convert from "local" program coordinates to other coordinate systems:
 * "Machine" coordinates are the actual machine axis positions (relative to their home position). For a milling machine, this would be the location of the spindle nose in the work envelope.
 * "Local" coordinates are what the program is written in:  p_local = T_g92^-1 * T_g5x^-1 *  T_tool_offset^-1 * p_machine
 * Equivalently: p_machine = T_tool_offset * T_g5x * T_g92 * p_local
 * "Tool" coordinates are machine coordinates, with the tool offset subtracted away: p_tool = T_tool_offset^-1 * p_machine = T_g5x * T_g92 * p_local
 */
struct LocalToMachineTransform : public TransformBase
{
    ShiftTransform g92Offset;
    RotateShiftTransform g5xOffset;
    ShiftTransform toolOffset;
    LocalToMachineTransform();

    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

    virtual PositionArray forward_local_to_tool(PositionArray const &local) const;
    virtual PositionArray inverse_local_from_tool(PositionArray const &tool) const;

    virtual PositionArray forward_local_to_machine(PositionArray const &local) const {return forward(local);}
    virtual PositionArray inverse_local_from_machine(PositionArray const &machine) const {return inverse(machine);}

    virtual PositionArray forward_tool_to_machine(PositionArray const &tool) const;
    virtual PositionArray inverse_tool_from_machine(PositionArray const &machine) const;

    PositionArray swap_tool_offset(PositionArray const &current_local_pos, ShiftTransform const &new_tool_offset);
    PositionArray swap_g5x_offset(PositionArray const &current_local_pos, RotateShiftTransform const &new_g5x_offset);

    PositionArray add_tool_offset(PositionArray const &current_local_pos, ShiftTransform const &new_tool_offset);

    double findAxisoffset(size_t axis_id);

    bool rotationActive() const;

    void rescaleAllTransforms(UnitsTransform const &to_new_units);
    LocalToMachineTransform scale_by(UnitsTransform const &to_new_units) const;
};

void convertAxisUnits(RotateShiftTransform &out, const UnitsTransform & units);

bool isNonzero(RotateShiftTransform const &r);
#endif // COORDINATETRANSFORM_HH
