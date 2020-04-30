#ifndef PREVIEWKINEMATICS_HH
#define PREVIEWKINEMATICS_HH

#include <array>
#include "positionarray.hh"
struct PM_CARTESIAN;

using Array3 = std::array<double, 3>;

/**
 * Maps to the classes below (used when constructing a new kinematics object from Python API)
 * Note that
 */
enum class KinsType
{
    Naive,
    CanonicalRS274,
    RotaryA,
};


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

    /**
     * Defines pieces of a global view transform to rotate the model view to follow a particular reference.
     *
     * The rotation / center is designed to "un-rotate" the world for machines
     * that have trunion / tilt tables. It assumes that the machine rotation
     * axes intersect at a single center point.
     *
     * Different view-tracking effects can be created by using parts of this
     * transform. For example, applying just the rotation will give a
     * "table"-centric view on a trunion machine, where the work rotates, but the tool
     * does all the translating.
     */
    virtual void forward_world(PositionArray const &p, //!< Joint position array
                         Array3 &position_xyz, //!< world view offset position in xyz for tool-frame view
                         Array3 &rotation_xyz, //!< world view orientation, Euler XYZ convention (used for table-frame view)
                         Array3 &rotation_center //!< rotation center of tilt axes, used for table-frame view
                               );

    virtual std::string describe() const; // Pretty-print the kinematics configuration for user debugging
    virtual ~PreviewKinematicsBase() = default;
};

/**
 * Axes act as originally imagined in the RS274 spec:
 * 1) XYZ position absolutely in workspace
 * 2) ABC rotates the tool about its tip around each global axis (in that order)
 * 3) UVW displaces the tool along the rotated axes.
 *
 * These are the kinematics of the "ideal" machine.
 */
using Kinematics_CanonicalRS274 = PreviewKinematicsBase;

/**
 * Defines kinematics for a 4-axis machine with a fixed A axis
 * parallel with the machine X axis.
 * No Tool Point Compensation (TPC) is currently supported. The A axis is at a fixed
 * location and orientation in the workspace.
 */
class Kinematics_RotaryA : public PreviewKinematicsBase
{
    double a_y0; // Absolute Y coordinate of A axis intercept in workspace
    double a_z0; // Absolute Z coordinate of A axis intercept in workspace
    double orientation; // +1 for right-facing A axis, -1 for left-facing
public:
    Kinematics_RotaryA() = default;
    Kinematics_RotaryA(double y0, double z0, bool flipped) : a_y0(y0), a_z0(z0), orientation(flipped ? -1.0 : 1.0) {}

    virtual void forward(PositionArray const &p, Array3 &position_xyz, Array3 &rotation_xyz);
    virtual void forward_position(PositionArray const &p, Array3 &position_xyz);
    virtual void forward_world(PositionArray const &p,
                         Array3 &position_xyz,
                         Array3 &rotation_xyz,
                         Array3 &rotation_center);
    virtual std::string describe() const; // Pretty-print the kinematics configuration for user debugging
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
class NaiveSerialKinematics : public PreviewKinematicsBase
{
    std::string geometry_str;
public:
    NaiveSerialKinematics() = default;
    NaiveSerialKinematics(const char *geometry_spec) : geometry_str(geometry_spec) {}
    virtual void forward(PositionArray const &p, Array3 &position_xyz, Array3 &rotation_xyz);
    virtual void forward_position(PositionArray const &pt, Array3 &p);
    virtual std::string describe() const; // Pretty-print the kinematics configuration for user debugging
};

#endif // PREVIEWKINEMATICS_HH
