#ifndef PREVIEWKINEMATICS_HH
#define PREVIEWKINEMATICS_HH

#include <array>
#include "positionarray.hh"
struct PM_CARTESIAN;

using Array3 = std::array<double, 3>;

class PreviewKinematicsBase
{
public:
    virtual void forward(PositionArray const &p, //!< Joint position array
                         Array3 &position_xyz, //!< tool position, xyz
                         Array3 &rotation_xyz //!< tool orientation, Euler XYZ convention
                         );
    virtual void forward_position(PositionArray const &p,
                                  Array3 &position_xyz //!< tool position, xyz
                                  );
    virtual ~PreviewKinematicsBase() = default;
};

using Kinematics_XYZ = PreviewKinematicsBase;

/**
 * Defines kinematics for a 4-axis machine with a fixed A axis
 * parallel with the machine X axis.
 * No Tool Point Compensation (TPC) is currently supported. The A axis is at a fixed
 * location and orientation in the workspace.
 */
class Kinematics_XYZA : public PreviewKinematicsBase
{
    double a_y0; // Absolute Y coordinate of A axis intercept in workspace
    double a_z0; // Absolute Z coordinate of A axis intercept in workspace
    double orientation; // +1 for right-facing A axis, -1 for left-facing
public:
    Kinematics_XYZA() = default;
    Kinematics_XYZA(double y0, double z0, bool flipped) : a_y0(y0), a_z0(z0), orientation(flipped ? -1.0 : 1.0) {}

    virtual void forward(PositionArray const &p, Array3 &position_xyz, Array3 &rotation_xyz);
    virtual void forward_position(PositionArray const &p, Array3 &position_xyz);
};

/**
 * General serial kinematics class that implements kinematics based on a
 * provided geometry string (based on the original emcmododule approach).
 * @note the kinematics are only correct if:
 * 1) All rotary machine axes intersect at a common point
 * 2) The local coordinate origin lies at that common point
 *
 * If either of these conditions aren't met, then the preview and backplot will
 * not be a faithful representation.
 */
class TrivialKinematics : public PreviewKinematicsBase
{
    std::string geometry_str;
public:
    TrivialKinematics() = default;
    TrivialKinematics(const char *geometry_spec) : geometry_str(geometry_spec) {}
    virtual void forward(PositionArray const &p, Array3 &position_xyz, Array3 &rotation_xyz);
    virtual void forward_position(PositionArray const &pt, Array3 &p);
};

#endif // PREVIEWKINEMATICS_HH
