/**
 * @file positionarray.cc
 * Helper functions for PositionArray type.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#include "positionarray.hh"
#include <cmath>
#include <algorithm>
#include <emcpos.h>
#include <algorithm>

PositionArray toArray(double _x, double _y, double _z, double _a, double _b, double _c, double _u, double _v, double _w)
{
    return {_x, _y, _z, _a, _b, _c, _u, _v, _w};
}

PositionArray toArray(const EmcPose & p)
{
    return {
        p.tran.x,
        p.tran.y,
        p.tran.z,
        p.a,
        p.b,
        p.c,
        p.u,
        p.v,
        p.w,
    };
}

void extract_axes(PositionArray const &p,
 double &_x,
 double &_y,
 double &_z,
 double &_a,
 double &_b,
 double &_c,
 double &_u,
 double &_v,
 double &_w)
{
    _x = p[X];
    _y = p[Y];
    _z = p[Z];
    _a = p[A];
    _b = p[B];
    _c = p[C];
    _u = p[U];
    _v = p[V];
    _w = p[W];
}

void extract_axes(const PositionArray & p, double * _x, double * _y, double * _z, double * _a, double * _b, double * _c, double * _u, double * _v, double * _w)
{
    // FIXME no null checks here! be sure you know what you're doing!!
    extract_axes(p, *_x, *_y, *_z, *_a, *_b, *_c, *_u, *_v, *_w);
}

PositionArray abs(const PositionArray & p)
{
    PositionArray out;
    for (size_t k=0; k < MAX_USED_AXES; ++k) {
        out[k] = fabs(p[k]);
    }
    return out;
}

PositionArray abs_diff(const PositionArray & b, const PositionArray & o)
{
    return abs(b - o);
}

double max(const PositionArray & p)
{
    return *std::max_element(p.begin(), p.end());
}

double min(const PositionArray & p)
{
    return *std::min_element(p.begin(), p.end());
}

EmcPose toEmcPose(const PositionArray & p)
{
    return {
            {
            p[X],
            p[Y],
            p[Z]},
        p[A],
        p[B],
        p[C],
        p[U],
        p[V],
        p[W]
    };
}

PositionArray merge(const PositionArray & old_p, const PositionArray & new_p, const AxisMask & new_mask)
{
    PositionArray out{};
    for (size_t k = 0; k < MAX_USED_AXES; ++k) {
        out[k] = new_mask.test(k) ? new_p[k] : old_p[k];
    }
    return out;
}

PositionArray toArray(double const *arr_begin, size_t count)
{
    PositionArray out{};
    for (size_t k = 0; k < count; ++k) {
        out[k] = arr_begin[k];
    }
    return out;
}

PositionArray toArray(double const *arr_begin, double const *arr_end)
{
    PositionArray out;
    std::copy(arr_begin, arr_end, out.begin());
    return out;
}

bool isNonzero(PositionArray const &p)
{
    bool out = false;
    for (size_t k = 0; k < MAX_USED_AXES; ++k) {
        out |= (p[k] != 0.0);
    }
    return out;
}

PositionArray max(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        out[i] = std::max(p0[i], p1[i]);
    }
    return out;
}

PositionArray min(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        out[i] = std::min(p0[i], p1[i]);
    }
    return out;
}

PositionArray & max_inplace(PositionArray & p0, const PositionArray & p1)
{
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        p0[i] = std::max(p0[i], p1[i]);
    }
    return p0;
}

PositionArray & min_inplace(PositionArray & p0, const PositionArray & p1)
{
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        p0[i] = std::min(p0[i], p1[i]);
    }
    return p0;
}

char axis_to_char(int ax)
{
    switch (ax) {
    case X:
    case Y:
    case Z:
    case A:
    case B:
    case C:
    case U:
    case V:
    case W:
        return AxisNames[ax];
    }
    return '_';
}

void apply_abs_mask(PositionArray & p, const PositionArray & min_mask)
{
    for (size_t i=0; i < p.size(); ++i) {
        if (p[i] < min_mask[i]) {
            p[i] = 0;
        }
    }
}

double dot(const PositionArray & p0, const PositionArray & p1)
{
    double d = 0;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        d += p0[i]*p1[i];
    }
    return d;
}

PositionArray &operator*=(PositionArray & p0, double v)
{
    for (auto &p : p0) {
        p*=v;
    }
    return p0;
}

PositionArray operator*(PositionArray const & p0, double v)
{
    PositionArray out = p0;
    for (auto &p : out) {
        p*=v;
    }
    return out;
}

double mag(const PositionArray & p0)
{
    return sqrt(dot(p0, p0));
}
