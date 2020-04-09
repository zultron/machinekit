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


RotateShiftTransform::RotateShiftTransform(const PositionArray & pos, double xy_rot_deg) :
    shift(pos)
{
    setXYRotationDeg(xy_rot_deg);
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

ShiftTransform UnitsTransform::forward_tf(const ShiftTransform & p) const
{
    return ShiftTransform(forward(p.tf));
}

ShiftTransform UnitsTransform::inverse_tf(const ShiftTransform & p) const
{
    return ShiftTransform(inverse(p.tf));
}

RotateShiftTransform UnitsTransform::forward_tf(const RotateShiftTransform & p) const
{
    return RotateShiftTransform(forward(p.shift.tf), p.getXYRotationDeg());
}

RotateShiftTransform UnitsTransform::inverse_tf(const RotateShiftTransform & p) const
{
    return RotateShiftTransform(inverse(p.shift.tf), p.getXYRotationDeg());
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

/**
 * Transform from local coordinates to tool tip coordinates.
 * tool tip coordinates are
 */
PositionArray LocalToMachineTransform::forward_local_to_tool(const PositionArray & local) const
{
     return g5xOffset.forward(g92Offset.forward(local));
}

/**
 * Transform from tool tip coordinates back to local coordinates.
 */
PositionArray LocalToMachineTransform::inverse_local_from_tool(const PositionArray & tool) const
{
    return g92Offset.inverse(g5xOffset.inverse(tool));
}

PositionArray LocalToMachineTransform::forward_tool_to_machine(const PositionArray & tool) const
{
    return toolOffset.forward(tool);
}

PositionArray LocalToMachineTransform::inverse_tool_from_machine(const PositionArray & machine) const
{
    return toolOffset.inverse(machine);
}

PositionArray LocalToMachineTransform::swap_tool_offset(const PositionArray & current_local_pos, const ShiftTransform & new_tool_offset)
{
    // Go forward to real machine coordinates with the current tool offset
    PositionArray machine_pos = forward(current_local_pos);
    // Swap out the tool offset
    toolOffset = new_tool_offset;
    // Apply inverse transform to get back to the current position with the new tool offset applied
    return inverse(machine_pos);
}

PositionArray LocalToMachineTransform::swap_g5x_offset(const PositionArray & current_local_pos, const RotateShiftTransform & new_g5x_offset)
{
    // Go forward to real tool coordinates with the current g5x offset
    PositionArray tool_pos = forward_local_to_tool(current_local_pos);
    // Swap out the g5x offset
    g5xOffset = new_g5x_offset;
    // Apply inverse transform to get the local position resulting from the new g5x offset
    return inverse_local_from_tool(tool_pos);
}


/**
 * Adds the new tool offset to the active offset, and returns an updated local position.
 */
PositionArray LocalToMachineTransform::add_tool_offset(const PositionArray & current_local_pos, const ShiftTransform & new_tool_offset)
{
    ShiftTransform combined_tool_offset(new_tool_offset.tf + toolOffset.tf);
    return swap_tool_offset(current_local_pos, combined_tool_offset);
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

LocalToMachineTransform LocalToMachineTransform::scale_by(const UnitsTransform & to_new_units) const
{
    auto transform_out = *this;
    transform_out.rescaleAllTransforms(to_new_units);
    return transform_out;
}

/**
 * Applies unit conversion supplied to linear / angular axes.
 * @note coordinate system rotation is always stored internally as radians, so this is NOT converted.
 */
void convertAxisUnits(RotateShiftTransform & out, const UnitsTransform & units)
{
    units.apply_fwd(out.shift.tf);
}

bool isNonzero(const RotateShiftTransform & r)
{
    return isNonzero(r.shift.tf) || r.xy_rotation_rad != 0.0;
}
