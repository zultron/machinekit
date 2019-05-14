/**
 * @file coordinatetransform.cc
 * Methods for coordinate transform API used in interp / emccanon.
 *
 * Typically, internals of the various transforms are public because the
 * interpreter has to do incremental updates. In some cases we can't do any
 * better encapsulation-wise than what G-code / canon API allows. It's much
 * easier to just directly poke an offset than try to anticipate all the
 * possible changes with a restricted public API.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

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

PositionArray TransformBase::operator()(const PositionArray & p) const
{
  return forward(p);
}

PositionArray TransformBase::forward_relative(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_fwd_relative(tmp);
    return tmp;
}

PositionArray TransformBase::inverse_relative(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_inv_relative(tmp);
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

PositionArray::value_type & ShiftTransform::operator[](PositionArrayIndex idx)
{
    return tf[idx];
}

PositionArray::value_type const & ShiftTransform::operator[](PositionArrayIndex idx) const
{
    return tf[idx];
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

void RotateShiftTransform::apply_fwd(PositionArray & p) const
{
    apply_fwd_relative(p);
    shift.apply_fwd(p);
}

void RotateShiftTransform::apply_inv(PositionArray & p) const
{
    shift.apply_inv(p);
    apply_inv_relative(p);
}

void RotateShiftTransform::apply_fwd_relative(PositionArray & p) const
{
    rotate_xy(p[X], p[Y]);
}

void RotateShiftTransform::apply_inv_relative(PositionArray & p) const
{
    unrotate_xy(p[X], p[Y]);
}


double RotateShiftTransform::getXYRotationDeg() const
{
    return xy_rotation_rad * 180.0 / M_PI;
}

void RotateShiftTransform::setXYRotationDeg(double xy_rot_deg)
{
    xy_rotation_rad = xy_rot_deg * M_PI / 180.0;
}

void RotateShiftTransform::rotate_xy(double &x, double &y) const {
    rotate(x, y, xy_rotation_rad);
}

void RotateShiftTransform::unrotate_xy(double & x, double & y) const
{
    rotate(x, y, -xy_rotation_rad);
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

LocalToMachineTransform::LocalToMachineTransform() :
  g92Offset(),
  g5xOffset(),
  toolOffset()
{
}

UnitsTransform & UnitsTransform::invert_scale()
{
    linear_fwd = 1.0 / linear_fwd;
    angular_fwd = 1.0 / angular_fwd;
    return *this;
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

// Only g5x has rotation (and nothing has scale)
void LocalToMachineTransform::apply_fwd_relative(PositionArray & p) const
{
    g5xOffset.apply_fwd_relative(p);
}

void LocalToMachineTransform::apply_inv_relative(PositionArray & p) const
{
    g5xOffset.apply_inv_relative(p);
}

/**
 * Transform from local coordinates to spindle "nose" coordinate (i.e. tool coordinates without tool offsets).
 */
PositionArray LocalToMachineTransform::forward_to_nose(const PositionArray & local) const
{
     return g5xOffset.forward(g92Offset.forward(local));
}

/**
 * Transform from nose coordinates back to local coordinates.
 */
PositionArray LocalToMachineTransform::inverse_from_nose(const PositionArray & nose) const
{
     return g92Offset.inverse(g5xOffset.inverse(nose));
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

void LocalToMachineTransform::rescaleAllTransforms(const UnitsTransform & to_new_units)
{
    to_new_units.apply_fwd(g92Offset.tf);
    to_new_units.apply_fwd(g5xOffset.shift.tf);
    to_new_units.apply_fwd(toolOffset.tf);
}
