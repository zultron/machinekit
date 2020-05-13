#include "nml_prettyprint.hh"
#include "emc_nml.hh"

#include <iostream>
#include <algorithm>
#include <iterator>

std::ostream &operator<<(std::ostream &out, EmcPose const &p)
{
    out << "[" << p.tran.x << ", "
        << p.tran.y << ", "
        << p.tran.z << ", "
        << p.a << ", "
        << p.b << ", "
        << p.c << ", "
        << p.u << ", "
        << p.v << ", "
        << p.w << "]";
    return out;
}

std::ostream & operator<<(std::ostream & out, const PositionArray & p)
{
    out << "[";
    std::copy(p.begin(), p.end(), std::ostream_iterator<double>(out, ", "));
    out << "]";
    return out;
}

std::ostream & operator<<(std::ostream & out, const ShiftTransform & shift_tf)
{
    out << shift_tf.tf;
    return out;
}

std::ostream & operator<<(std::ostream & out, const RotateShiftTransform & rotshift_tf)
{
    out << "[" << rotshift_tf.shift.tf << ", " << rotshift_tf.getXYRotationDeg() << " deg]";
    return out;
}

std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_G92 const &msg)
{
    out << "G92_Offset: " << msg.origin;
    return out;
}

std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_G5X_TRANSFORM const &msg)
{
    out << "G5x_Offset: " << msg.g5x_transform;
    return out;
}

std::ostream &to_stream(std::ostream &out, EMC_TRAJ_SET_TOOL_OFFSET const &msg)
{
    out << "Tool_Offset: " << msg.offset;
    return out;
}
