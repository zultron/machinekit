#include "previewkinematics.hh"
#include "posemath.h"
#include "math.h"

/**
 * Default kinematics implementation is just trivial XYZ kinematics, ignoring all other axes
 */
void PreviewKinematicsBase::forward(const PositionArray & p, Array3 &position_xyz, Array3 &rotation_xyz)
{
    forward_position(p, position_xyz);
    // Following old live plot example, not sure why B is flipped
    rotation_xyz[X] = p[A] * M_PI / 180;
    rotation_xyz[Y] = -p[B] * M_PI / 180;
    rotation_xyz[Z] = p[C] * M_PI / 180;
}

void PreviewKinematicsBase::forward_position(const PositionArray & p, Array3 &position_xyz)
{
    std::copy(p.begin(), p.begin()+3, position_xyz.begin());
}

void PreviewKinematicsBase::forward_world(const PositionArray & p, Array3 &position_xyz, Array3 &rotation_xyz, Array3 &rotation_center)
{
    position_xyz.fill(0);
    rotation_xyz.fill(0);
    rotation_center.fill(0);
}

void Kinematics_XYZA::forward(const PositionArray & p, Array3 &position_xyz, Array3 &rotation_xyz)
{
    forward_position(p, position_xyz);
    rotation_xyz[X] = orientation * p[A] * M_PI / 180;
    rotation_xyz[Y] = 0.0;
    rotation_xyz[Z] = 0.0;
}

void Kinematics_XYZA::forward_position(const PositionArray &p, Array3 &position_xyz)
{
    double dY = p[Y] - a_y0;
    double dZ = p[Z] - a_z0;
    double a_rad = orientation * p[A] * M_PI / 180.;
    double cA = cos(a_rad), sA = sin(a_rad);

    position_xyz[X] = p[X];
    position_xyz[Y] = a_y0 + dY * cA - dZ * sA;
    position_xyz[Z] = a_z0 + dY * sA + dZ * cA;
}

void Kinematics_XYZA::forward_world(const PositionArray & p, Array3 &position_xyz, Array3 &rotation_xyz, Array3 &rotation_center)
{
    rotation_xyz[X] = -orientation * p[A] * M_PI / 180.;

    rotation_center[X] = 0.0;
    rotation_center[Y] = a_y0;
    rotation_center[Z] = a_z0;

    position_xyz[X] = -p[X];
    position_xyz[Y] = -p[Y];
    position_xyz[Z] = 0;
}

static void rotate_z(Array3 &pt, double a) {
    double theta = a * M_PI / 180;
    double c = cos(theta), s = sin(theta);
    double tx, ty;
    tx = pt[0] * c - pt[1] * s;
    ty = pt[0] * s + pt[1] * c;

    pt[0] = tx; pt[1] = ty;
}

static void rotate_y(Array3 &pt, double a) {
    double theta = a * M_PI / 180;
    double c = cos(theta), s = sin(theta);
    double tx, tz;
    tx = pt[0] * c - pt[2] * s;
    tz = pt[0] * s + pt[2] * c;

    pt[0] = tx; pt[2] = tz;
}

static void rotate_x(Array3 &pt, double a) {
    double theta = a * M_PI / 180;
    double c = cos(theta), s = sin(theta);
    double tx, tz;
    tx = pt[1] * c - pt[2] * s;
    tz = pt[1] * s + pt[2] * c;

    pt[1] = tx; pt[2] = tz;
}

void TrivialKinematics::forward(const PositionArray & pt, Array3 &p, Array3 &rotation_xyz)
{
    double sign = 1;
    size_t r_idx=0;

    p = {0,0,0};
    rotation_xyz = {0,0,0};
    static constexpr double d2r = M_PI / 180;

    const char *geometry = geometry_str.c_str();
    for(; *geometry; geometry++) {
        switch(*geometry) {
            case '-': sign = -1; break;
            case 'X': p[X] += pt[X]*sign; sign=1; break;
            case 'Y': p[Y] += pt[Y]*sign; sign=1; break;
            case 'Z': p[Z] += pt[Z]*sign; sign=1; break;
            case 'U': p[X] += pt[U]*sign; sign=1; break;
            case 'V': p[Y] += pt[V]*sign; sign=1; break;
            case 'W': p[Z] += pt[W]*sign; sign=1; break;
            case 'A':
                rotate_x(p, pt[A] * sign);
                rotation_xyz[r_idx++ % 3] = pt[A] * sign * d2r;
                sign=1;
                break;
            case 'B':
                rotate_y(p, pt[B] * sign);
                rotation_xyz[r_idx++ % 3] = pt[B] * sign * d2r;
                sign=1;
                break;
            case 'C':
                rotate_z(p, pt[C] * sign);
                rotation_xyz[r_idx++ % 3] = pt[C] * sign * d2r;
                sign=1;
                break;
        }
    }
}

void TrivialKinematics::forward_position(const PositionArray & pt, Array3 &p)
{
    double sign = 1;

    p[0] = 0;
    p[1] = 0;
    p[2] = 0;

    const char *geometry = geometry_str.c_str();
    for(; *geometry; geometry++) {
        switch(*geometry) {
            case '-': sign = -1; break;
            case 'X': p[X] += pt[X]*sign; sign=1; break;
            case 'Y': p[Y] += pt[Y]*sign; sign=1; break;
            case 'Z': p[Z] += pt[Z]*sign; sign=1; break;
            case 'U': p[X] += pt[U]*sign; sign=1; break;
            case 'V': p[Y] += pt[V]*sign; sign=1; break;
            case 'W': p[Z] += pt[W]*sign; sign=1; break;
            case 'A': rotate_x(p, pt[A] * sign); sign=1; break;
            case 'B': rotate_y(p, pt[B] * sign); sign=1; break;
            case 'C': rotate_z(p, pt[C] * sign); sign=1; break;
        }
    }
}
