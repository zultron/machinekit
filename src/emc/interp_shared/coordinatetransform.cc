#include "coordinatetransform.hh"
#include <math.h>

static void rotate(double &x, double &y, double theta_rad) {
    if (theta_rad == 0.0) {
        return;
    }
    const double x_in = x, y_in = y;
    const double C = cos(theta_rad);
    const double S = sin(theta_rad);
    x = x_in * C - y_in * S;
    y = x_in * S + y_in * C;
}

PositionArray TransformBase::forward(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_fwd(tmp);
    return tmp;
}

PositionArray TransformBase::inverse(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_inv(tmp);
    return tmp;
}

ShiftTransform::ShiftTransform() :
    tf()
{
}

ShiftTransform::ShiftTransform(const PositionArray & pos) :
    tf(pos)
{
}

const PositionArray & ShiftTransform::getOffset() const
{
    return tf;
}

void ShiftTransform::apply_fwd(PositionArray & p) const
{
    p += tf;
}

void ShiftTransform::apply_inv(PositionArray & p) const
{
    p -= tf;
}


RotateShiftTransform::RotateShiftTransform() :
    shift(),
    xy_rotation_rad()
{
}

RotateShiftTransform::RotateShiftTransform(const PositionArray & pos, double xy_rot_rad) :
    shift(pos), xy_rotation_rad(xy_rot_rad)
{
}

const PositionArray & RotateShiftTransform::getOffset() const
{
    return shift.getOffset();
}

double RotateShiftTransform::getXYRotationDeg() const
{
    return xy_rotation_rad * 180.0 / M_PI;
}

void RotateShiftTransform::rotate_xy(double &x, double &y) const {
    rotate(x, y, xy_rotation_rad);
}

void RotateShiftTransform::unrotate_xy(double & x, double & y) const
{
    rotate(x, y, -xy_rotation_rad);
}

void RotateShiftTransform::apply_fwd(PositionArray & p) const
{
    rotate_xy(p[X], p[Y]);
    shift.apply_fwd(p);
}

void RotateShiftTransform::apply_inv(PositionArray & p) const
{
    shift.apply_inv(p);
    unrotate_xy(p[X], p[Y]);
}

void RotateShiftTransform::setShift(const PositionArray & p)
{
    shift = ShiftTransform(p);
}

void RotateShiftTransform::setXYRotationDeg(double xy_rot_deg)
{
    xy_rotation_rad = xy_rot_deg * M_PI / 180.0;
}

void UnitsTransform::scale(PositionArray & p, double linear_in, double angular_in)
{
    for (auto const& l : linearAxes) {
        p[l] *= linear_in;
    }
    for (auto const& a : angularAxes) {
        p[a] *= angular_in;
    }
}

UnitsTransform::UnitsTransform(double linear_in, double angular_in) :
    linear_fwd(linear_in),
    angular_fwd(angular_in)
{}

void UnitsTransform::apply_fwd(PositionArray & p) const {
    scale(p, linear_fwd, angular_fwd);
}

void UnitsTransform::apply_inv(PositionArray & p) const {
    scale(p, 1.0 / linear_fwd, 1.0 / angular_fwd);
}

void LocalToMachineTransform::apply_fwd(PositionArray & p) const
{
    g92Offset.apply_fwd(p);
    g5xOffset.apply_fwd(p);
    toolOffset.apply_fwd(p);
}

void LocalToMachineTransform::apply_inv(PositionArray & p) const
{
    toolOffset.apply_inv(p);
    g5xOffset.apply_inv(p);
    g92Offset.apply_inv(p);
}

double LocalToMachineTransform::findAxisoffset(size_t axis_id)
{
    // KLUDGE inefficient implementation (lots of redundant work here)
    PositionArray tmp=PositionArray();
    apply_fwd(tmp);
    return tmp[axis_id];
}

bool LocalToMachineTransform::rotationActive() const
{
    return g5xOffset.getXYRotationDeg() != 0.0;
}
