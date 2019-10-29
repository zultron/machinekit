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
    ShiftTransform(PositionArray const &pos);

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

    UnitsTransform &invert_scale();
};

struct LocalToMachineTransform : public TransformBase
{
    ShiftTransform g92Offset;
    RotateShiftTransform g5xOffset;
    ShiftTransform toolOffset;
    LocalToMachineTransform();

    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

    virtual PositionArray forward_to_nose(PositionArray const &local) const;
    virtual PositionArray inverse_from_nose(PositionArray const &nose) const;

    virtual PositionArray forward_to_tip(PositionArray const &p) const {return forward(p);}
    virtual PositionArray inverse_from_tip(PositionArray const &p) const {return inverse(p);}

    PositionArray swap_tool_offset(PositionArray const &current_local_pos, ShiftTransform const &new_tool_offset);
    PositionArray swap_g5x_offset(PositionArray const &current_local_pos, RotateShiftTransform const &new_g5x_offset);

    PositionArray add_tool_offset(PositionArray const &current_local_pos, ShiftTransform const &new_tool_offset);

    double findAxisoffset(size_t axis_id);

    bool rotationActive() const;

    void rescaleAllTransforms(UnitsTransform const &to_new_units);
};

void convertAxisUnits(RotateShiftTransform &out, const UnitsTransform & units);

bool isNonzero(RotateShiftTransform const &r);
#endif // COORDINATETRANSFORM_HH
