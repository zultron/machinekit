#ifndef ROTARY_AXIS_UTIL_HH
#define ROTARY_AXIS_UTIL_HH

#include <positionarray.hh>

PositionArray & wrap_rotary_axes(PositionArray & p,
                             bool A_axis_wrapped,
                             bool B_axis_wrapped,
                             bool C_axis_wrapped);

PositionArray & unwrap_rotary_axes(PositionArray & p,
                             bool A_axis_wrapped,
                             bool B_axis_wrapped,
                             bool C_axis_wrapped);
double wrap_rotary(double r);
double unwrap_rotary(double direction, double command, double current, char axis);

/** Contains shared (effectively constant) properties used for wrap / unwrapping axes.*/
struct WrappedRotaryBase
{
    WrappedRotaryBase();
    WrappedRotaryBase(bool a_wrapped, bool b_wrapped, bool c_wrapped);
    bool a_axis_wrapped;
    bool b_axis_wrapped;
    bool c_axis_wrapped;
};

struct WrapsRotaryAxes : public WrappedRotaryBase
{
    WrapsRotaryAxes(bool a_wrapped, bool b_wrapped, bool c_wrapped) :
        WrappedRotaryBase(a_wrapped, b_wrapped, c_wrapped) {}

    PositionArray operator()(PositionArray const &current) const;
};

struct UnwrapsRotaryAxes : public WrappedRotaryBase
{
    UnwrapsRotaryAxes(bool a_wrapped, bool b_wrapped, bool c_wrapped) :
        WrappedRotaryBase(a_wrapped, b_wrapped, c_wrapped) {}

    PositionArray operator()(PositionArray const &direction, PositionArray const &command, PositionArray const &current) const;
};


#endif // ROTARY_AXIS_UTIL_HH
