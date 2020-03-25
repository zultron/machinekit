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
#include <emcpose.h>
#include <algorithm>


PositionArray & operator+=(PositionArray & b, const PositionArray & o)
{
    for (size_t k=0; k <b.size(); ++k) {
        b[k] += o[k];
    }
    return b;
}

PositionArray & operator-=(PositionArray & b, const PositionArray & o)
{
    for (size_t k=0; k <b.size(); ++k) {
        b[k] -= o[k];
    }
    return b;

}

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

PositionArray operator+(const PositionArray & b, const PositionArray & o)
{
    PositionArray out = b;
    return out += o;
}

PositionArray operator-(const PositionArray & b, const PositionArray & o)
{
    PositionArray out = b;
    return out -= o;
}

PositionArray & operator*=(PositionArray & b, double d)
{
    for (size_t k=0; k <b.size(); ++k) {
        b[k] *= d;
    }
    return b;
}

PositionArray operator*(const PositionArray & b, double d)
{
    PositionArray out = b;
    return out *= d;
}

PositionArray & operator/=(PositionArray & b, double d)
{
    for (size_t k=0; k <b.size(); ++k) {
        b[k] /= d;
    }
    return b;

}

PositionArray operator/(const PositionArray & b, double d)
{
    PositionArray out = b;
    return out /= d;
}

PositionArray abs(const PositionArray & p)
{
    PositionArray out;
    for (size_t k=0; k <p.size(); ++k) {
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

void merge_masked(PositionArray & p, const PositionArray & in, const AxisMask & new_value_mask)
{
    for (size_t k = 0; k < in.size(); ++k) {
        if (new_value_mask[k]) {
            p[k] = in[k];
        }
    }
}

PositionArray merge(const PositionArray & old_p, const PositionArray & new_p, const AxisMask & new_mask)
{
    PositionArray out = old_p;
    merge_masked(out, new_p, new_mask);
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
  return
      p[X] != 0.0 ||
      p[Y] != 0.0 ||
      p[Z] != 0.0 ||
      p[A] != 0.0 ||
      p[B] != 0.0 ||
      p[C] != 0.0 ||
      p[U] != 0.0 ||
      p[V] != 0.0 ||
      p[W] != 0.0;
}

PositionArray max(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out{};
    for (size_t i=0; i < p0.size(); ++i) {
        out[i] = std::max(p0[i], p1[i]);
    }
    return out;
}

PositionArray min(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out{};
    for (size_t i=0; i < p0.size(); ++i) {
        out[i] = std::min(p0[i], p1[i]);
    }
    return out;
}
