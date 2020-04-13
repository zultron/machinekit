#include "previewkinematics.hh"
#include "posemath.h"
#include "math.h"

void Kinematics_XYZA::forward(const PositionArray & p, Array3 &out)
{
    double dY = p[Y] - a_y0;
    double dZ = p[Z] - a_z0;
    double a_rad = orientation * p[A] * M_PI / 180.;
    double cA = cos(a_rad), sA = sin(a_rad);

    out[X] = p[X];
    out[Y] = a_y0 + dY * cA - dZ * sA;
    out[Z] = a_z0 + dY * sA + dZ * cA;
}

/**
 * Default kinematics implementation is just trivial XYZ kinematics, ignoring all other axes
 */
void PreviewKinematicsBase::forward(const PositionArray & p, Array3 &out)
{
    std::copy(p.begin(), p.begin()+3, out.begin());
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

void TrivialKinematics::forward(const PositionArray & pt, Array3 & p)
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
