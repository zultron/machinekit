#ifndef COORDINATETRANSFORM_HH
#define COORDINATETRANSFORM_HH

#include <positionarray.hh>

struct TransformBase
{
    // Includes absolute offsets
    virtual void apply_fwd(PositionArray &p) const = 0;
    virtual void apply_inv(PositionArray &p) const = 0;

    // Ignores absolute offsets
    virtual void apply_fwd_relative(PositionArray &) const {}
    virtual void apply_inv_relative(PositionArray &) const {}

    virtual PositionArray forward(PositionArray const &p) const;
    virtual PositionArray inverse(PositionArray const &p) const;

    virtual PositionArray operator()(PositionArray const &p) const;

    virtual PositionArray forward_relative(PositionArray const &p) const;
    virtual PositionArray inverse_relative(PositionArray const &p) const;

    virtual ~TransformBase() = default;
};

struct ShiftTransform : public TransformBase
{
    PositionArray tf;

    ShiftTransform();
    ShiftTransform(PositionArray const &pos);

    PositionArray const &getOffset() const;

    // All 9 axes
    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

};

struct RotateShiftTransform : public TransformBase
{
    ShiftTransform shift;
    double xy_rotation_rad;

    RotateShiftTransform();
    RotateShiftTransform(PositionArray const &pos,
                         double xy_rot_rad);

    const PositionArray &getOffset() const;
    double getXYRotationDeg() const;

    void apply_fwd(PositionArray &p) const;
    void apply_inv(PositionArray &p) const;

    void apply_fwd_relative(PositionArray &p) const;
    void apply_inv_relative(PositionArray &p) const;

    void setShift(PositionArray const &p);
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

    // Passthrough
    void apply_fwd_relative(PositionArray &p) const { apply_fwd(p);}
    void apply_inv_relative(PositionArray &p) const { apply_inv(p);}

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

    void apply_fwd_relative(PositionArray &p) const;
    void apply_inv_relative(PositionArray &p) const;

    virtual PositionArray forward_to_nose(PositionArray const &local) const;
    virtual PositionArray inverse_from_nose(PositionArray const &nose) const;

    virtual PositionArray forward_to_tip(PositionArray const &p) const {return forward(p);}
    virtual PositionArray inverse_from_tip(PositionArray const &p) const {return inverse(p);}

    double findAxisoffset(size_t axis_id);

    bool rotationActive() const;

    void rescaleAllTransforms(UnitsTransform const &to_new_units);
};

#endif // COORDINATETRANSFORM_HH
