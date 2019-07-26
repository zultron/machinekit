#include "rotary_axis_util.hh"
#include <math.h>
#include <interp_return.hh>

PositionArray & wrap_rotary_axes(PositionArray & p,
                                    bool A_axis_wrapped,
                                    bool B_axis_wrapped,
                                    bool C_axis_wrapped)
{

    if(A_axis_wrapped) {
        wrap_rotary(p[A]);
    }
    if(B_axis_wrapped) {
        wrap_rotary(p[B]);
    }
    if(C_axis_wrapped) {
        wrap_rotary(p[C]);
    }
    return p;
}

/**
 * Flatten an arbitrary rotation to the range [0,360].
 */
double wrap_rotary(double r)
{
    r = fmod(r, 360.0);
    if(r < 0) {
      r += 360.0;
    }
    return r;
}

/**
 * Find the real destination, given the axis's current position, the commanded
 * destination, and the direction to turn (which comes from the sign of the
 * commanded value in the gcode).
 * Modulo 360 positions of the axis are
 * considered equivalent and we just need to find the nearest one.
 *
 * @note this preserves any existing rotations (e.g. current position could be
 * much greater than 360 deg), and applies a delta of no more than 360 deg to
 * the current position.
 */
double unwrap_rotary(double direction, double command, double current)
{
    const bool neg = direction < 0.0;

    double d = floor(current/360.0);
    double result = fabs(command) + (d*360.0);
    if(!neg && result < current) {result += 360.0;}
    if(neg && result > current) {result -= 360.0;}
    return result;
}

WrappedRotaryBase::WrappedRotaryBase() :
    WrappedRotaryBase(false, false, false)
{
}

WrappedRotaryBase::WrappedRotaryBase(bool a_wrapped, bool b_wrapped, bool c_wrapped) :
    a_axis_wrapped(a_wrapped), b_axis_wrapped(b_wrapped), c_axis_wrapped(c_wrapped)
{
}

PositionArray WrapsRotaryAxes::operator()(const PositionArray &current) const
{
    PositionArray p = current;
    if(a_axis_wrapped) {
        wrap_rotary(p[A]);
    }
    if(b_axis_wrapped) {
        wrap_rotary(p[B]);
    }
    if(c_axis_wrapped) {
        wrap_rotary(p[C]);
    }
    return p;
}

PositionArray UnwrapsRotaryAxes::operator()(const PositionArray &direction, const PositionArray &command, const PositionArray &current) const
{
    PositionArray p = current;
    if(a_axis_wrapped) {
        p[A] = unwrap_rotary(direction[A], command[A], p[A]);
    }
    if(b_axis_wrapped) {
        p[B] = unwrap_rotary(direction[B], command[B], p[B]);
    }
    if(c_axis_wrapped) {
        p[C] = unwrap_rotary(direction[C], command[C], p[C]);
    }
    return p;
}
