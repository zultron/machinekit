#ifndef COORDINATETRANSFORM_HH
#define COORDINATETRANSFORM_HH

#include <positionarray.hh>
#include <math.h>

namespace interp_shared {
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
    LocalToMachineTransform(ShiftTransform const &g92, RotateShiftTransform const &g5x, ShiftTransform const &tool);

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
    PositionArray swap_g92_offset(PositionArray const &current_local_pos, ShiftTransform const &new_g92_offset);

    PositionArray add_tool_offset(PositionArray const &current_local_pos, ShiftTransform const &new_tool_offset);

    double findAxisoffset(size_t axis_id);

    bool rotationActive() const;

    void rescaleAllTransforms(UnitsTransform const &to_new_units);
    LocalToMachineTransform scale_by(UnitsTransform const &to_new_units) const;
};

void convertAxisUnits(RotateShiftTransform &out, const UnitsTransform & units);

bool isNonzero(RotateShiftTransform const &r);

// Begin inline definitions for header-only library

inline static void rotate(double &x, double &y, double theta_rad) {
    if (theta_rad == 0.0) {
        return;
    }
    const double x_in = x, y_in = y;
    const double C = cos(theta_rad);
    const double S = sin(theta_rad);
    x = x_in * C - y_in * S;
    y = x_in * S + y_in * C;
}

inline PositionArray TransformBase::forward(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_fwd(tmp);
    return tmp;
}

inline PositionArray TransformBase::inverse(const PositionArray & p) const
{
    PositionArray tmp(p);
    apply_inv(tmp);
    return tmp;
}

inline PositionArray TransformBase::operator()(const PositionArray & p) const
{
  return forward(p);
}

inline ShiftTransform::ShiftTransform() :
    tf()
{
}

inline ShiftTransform::ShiftTransform(const PositionArray & pos) :
    tf(pos)
{
}

inline PositionArray::value_type & ShiftTransform::operator[](PositionArrayIndex idx)
{
    return tf[idx];
}

inline PositionArray::value_type const & ShiftTransform::operator[](PositionArrayIndex idx) const
{
    return tf[idx];
}

inline void ShiftTransform::apply_fwd(PositionArray & p) const
{
    p += tf;
}

inline void ShiftTransform::apply_inv(PositionArray & p) const
{
    p -= tf;
}


inline RotateShiftTransform::RotateShiftTransform(const PositionArray & pos, double xy_rot_deg) :
    shift(pos)
{
    setXYRotationDeg(xy_rot_deg);
}

inline void RotateShiftTransform::apply_fwd(PositionArray & p) const
{
    apply_fwd_relative(p);
    shift.apply_fwd(p);
}

inline void RotateShiftTransform::apply_inv(PositionArray & p) const
{
    shift.apply_inv(p);
    apply_inv_relative(p);
}

inline void RotateShiftTransform::apply_fwd_relative(PositionArray & p) const
{
    rotate_xy(p[X], p[Y]);
}

inline void RotateShiftTransform::apply_inv_relative(PositionArray & p) const
{
    unrotate_xy(p[X], p[Y]);
}


inline double RotateShiftTransform::getXYRotationDeg() const
{
    return xy_rotation_rad * 180.0 / M_PI;
}

inline void RotateShiftTransform::setXYRotationDeg(double xy_rot_deg)
{
    xy_rotation_rad = xy_rot_deg * M_PI / 180.0;
}

inline void RotateShiftTransform::rotate_xy(double &x, double &y) const {
    rotate(x, y, xy_rotation_rad);
}

inline void RotateShiftTransform::unrotate_xy(double & x, double & y) const
{
    rotate(x, y, -xy_rotation_rad);
}

inline void UnitsTransform::scale(PositionArray & p, double linear_in, double angular_in)
{
    for (auto const& l : linearAxes) {
        p[l] *= linear_in;
    }
    for (auto const& a : angularAxes) {
        p[a] *= angular_in;
    }
}

inline UnitsTransform::UnitsTransform(double linear_in, double angular_in) :
    linear_fwd(linear_in),
    angular_fwd(angular_in)
{}

inline void UnitsTransform::apply_fwd(PositionArray & p) const {
    scale(p, linear_fwd, angular_fwd);
}

inline void UnitsTransform::apply_inv(PositionArray & p) const {
    scale(p, 1.0 / linear_fwd, 1.0 / angular_fwd);
}

inline ShiftTransform UnitsTransform::forward_tf(const ShiftTransform & p) const
{
    return ShiftTransform(forward(p.tf));
}

inline ShiftTransform UnitsTransform::inverse_tf(const ShiftTransform & p) const
{
    return ShiftTransform(inverse(p.tf));
}

inline RotateShiftTransform UnitsTransform::forward_tf(const RotateShiftTransform & p) const
{
    return RotateShiftTransform(forward(p.shift.tf), p.getXYRotationDeg());
}

inline RotateShiftTransform UnitsTransform::inverse_tf(const RotateShiftTransform & p) const
{
    return RotateShiftTransform(inverse(p.shift.tf), p.getXYRotationDeg());
}


inline LocalToMachineTransform::LocalToMachineTransform() :
  g92Offset(),
  g5xOffset(),
  toolOffset()
{
}

inline LocalToMachineTransform::LocalToMachineTransform(const ShiftTransform & g92, const RotateShiftTransform & g5x, const ShiftTransform & tool) :
    g92Offset(g92),
    g5xOffset(g5x),
    toolOffset(tool)
{
}

inline UnitsTransform & UnitsTransform::invert_scale()
{
    linear_fwd = 1.0 / linear_fwd;
    angular_fwd = 1.0 / angular_fwd;
    return *this;
}

inline void LocalToMachineTransform::apply_fwd(PositionArray & p) const
{
    g92Offset.apply_fwd(p);
    g5xOffset.apply_fwd(p);
    toolOffset.apply_fwd(p);
}

inline void LocalToMachineTransform::apply_inv(PositionArray & p) const
{
    toolOffset.apply_inv(p);
    g5xOffset.apply_inv(p);
    g92Offset.apply_inv(p);
}

/**
 * Transform from local coordinates to tool tip coordinates.
 * tool tip coordinates are
 */
inline PositionArray LocalToMachineTransform::forward_local_to_tool(const PositionArray & local) const
{
     return g5xOffset.forward(g92Offset.forward(local));
}

/**
 * Transform from tool tip coordinates back to local coordinates.
 */
inline PositionArray LocalToMachineTransform::inverse_local_from_tool(const PositionArray & tool) const
{
    return g92Offset.inverse(g5xOffset.inverse(tool));
}

inline PositionArray LocalToMachineTransform::forward_tool_to_machine(const PositionArray & tool) const
{
    return toolOffset.forward(tool);
}

inline PositionArray LocalToMachineTransform::inverse_tool_from_machine(const PositionArray & machine) const
{
    return toolOffset.inverse(machine);
}

inline PositionArray LocalToMachineTransform::swap_tool_offset(const PositionArray & current_local_pos, const ShiftTransform & new_tool_offset)
{
    // Go forward to real machine coordinates with the current tool offset
    PositionArray machine_pos = forward(current_local_pos);
    // Swap out the tool offset
    toolOffset = new_tool_offset;
    // Apply inverse transform to get back to the current position with the new tool offset applied
    return inverse(machine_pos);
}

inline PositionArray LocalToMachineTransform::swap_g5x_offset(const PositionArray & current_local_pos, const RotateShiftTransform & new_g5x_offset)
{
    // Go forward to real tool coordinates with the current g5x offset
    PositionArray tool_pos = forward_local_to_tool(current_local_pos);
    // Swap out the g5x offset
    g5xOffset = new_g5x_offset;
    // Apply inverse transform to get the local position resulting from the new g5x offset
    return inverse_local_from_tool(tool_pos);
}

inline PositionArray LocalToMachineTransform::swap_g92_offset(const PositionArray & current_local_pos, const ShiftTransform & new_g92_offset)
{
    // Go forward to work coordinates (i.e. unapply the current G92 offset)
    PositionArray work_pos = g92Offset.forward(current_local_pos);
    // Swap out the g92 offset
    g92Offset = new_g92_offset;
    // Apply inverse transform to get back to local position resulting from the new g92 offset
    return g92Offset.inverse(work_pos);
}


/**
 * Adds the new tool offset to the active offset, and returns an updated local position.
 */
inline PositionArray LocalToMachineTransform::add_tool_offset(const PositionArray & current_local_pos, const ShiftTransform & new_tool_offset)
{
    ShiftTransform combined_tool_offset(new_tool_offset.tf + toolOffset.tf);
    return swap_tool_offset(current_local_pos, combined_tool_offset);
}

inline double LocalToMachineTransform::findAxisoffset(size_t axis_id)
{
    // KLUDGE inefficient implementation (lots of redundant work here)
    PositionArray tmp=PositionArray();
    apply_fwd(tmp);
    return tmp[axis_id];
}

inline bool LocalToMachineTransform::rotationActive() const
{
    return g5xOffset.getXYRotationDeg() != 0.0;
}

inline void LocalToMachineTransform::rescaleAllTransforms(const UnitsTransform & to_new_units)
{
    to_new_units.apply_fwd(g92Offset.tf);
    to_new_units.apply_fwd(g5xOffset.shift.tf);
    to_new_units.apply_fwd(toolOffset.tf);
}

inline LocalToMachineTransform LocalToMachineTransform::scale_by(const UnitsTransform & to_new_units) const
{
    auto transform_out = *this;
    transform_out.rescaleAllTransforms(to_new_units);
    return transform_out;
}

/**
 * Applies unit conversion supplied to linear / angular axes.
 * @note coordinate system rotation is always stored internally as radians, so this is NOT converted.
 */
inline void convertAxisUnits(RotateShiftTransform & out, const UnitsTransform & units)
{
    units.apply_fwd(out.shift.tf);
}

inline bool isNonzero(const RotateShiftTransform & r)
{
    return isNonzero(r.shift.tf) || r.xy_rotation_rad != 0.0;
}
}

#endif // COORDINATETRANSFORM_HH
