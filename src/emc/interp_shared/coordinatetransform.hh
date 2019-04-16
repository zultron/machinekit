#ifndef COORDINATETRANSFORM_HH
#define COORDINATETRANSFORM_HH

#include <positionarray.hh>

// TODO restore privacy / encapsulation after interp cleanup is done
struct TransformBase
{
    virtual void apply_fwd(PositionArray &p) const = 0;
    virtual void apply_inv(PositionArray &p) const = 0;

    virtual PositionArray forward(PositionArray const &p) const;
    virtual PositionArray inverse(PositionArray const &p) const;

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

    double findAxisoffset(size_t axis_id);

    bool rotationActive() const;
};

#endif // COORDINATETRANSFORM_HH
