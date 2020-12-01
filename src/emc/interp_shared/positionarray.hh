/**
 * @file positionarray.hh
 * Simple position array class to store a set of world axis positions in a std::array.
 * A std::vector would be overkill for this purpose (and add failure modes like
 * mismatching sizes). Think of this as a more flexible EmcPose.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#ifndef POSITIONARRAY_HH
#define POSITIONARRAY_HH

#include <cmath>
#include <algorithm>
#include <emcpos.h>

// interp_shared templates
#include <array_math.hh>
#include "interp_shared_fwd.hh"

namespace interp_shared {

char axis_to_char(int ax);

PositionArray abs(PositionArray const &p);
PositionArray abs_diff(PositionArray const &b, PositionArray const &o);
double max(PositionArray const &p);
double min(PositionArray const & p);
PositionArray &max_inplace(PositionArray &p0, PositionArray const &p1);
PositionArray &min_inplace(PositionArray &p0, PositionArray const &p1);
PositionArray max(PositionArray const &p0, PositionArray const &p1);
PositionArray min(PositionArray const &p0, PositionArray const &p1);

PositionArray &operator*=(PositionArray &p0, double v);
PositionArray operator*(PositionArray const &p0, double v);

double dot(PositionArray const &p0, PositionArray const &p1);
double mag(PositionArray const &p0);

void apply_abs_mask(PositionArray &p, PositionArray const &min_mask);

bool isNonzero(PositionArray const &p);

EmcPose toEmcPose(PositionArray const &p);

PositionArray merge(PositionArray const &old_p,
                   PositionArray const &new_p,
                   AxisMask const &new_mask);

PositionArray toArray(
        double _x,
        double _y,
        double _z,
        double _a,
        double _b,
        double _c,
        double _u,
        double _v,
        double _w);

PositionArray toArray(const double * arr_begin,
        size_t count);

PositionArray toArray(const double * arr_begin,
        const double * arr_end);

PositionArray toArray(EmcPose const &p);

void extract_axes(PositionArray const &p,
                  double &_x,
                  double &_y,
                  double &_z,
                  double &_a,
                  double &_b,
                  double &_c,
                  double &_u,
                  double &_v,
                  double &_w);

void extract_axes(PositionArray const &p,
                  double *_x,
                  double *_y,
                  double *_z,
                  double *_a,
                  double *_b,
                  double *_c,
                  double *_u,
                  double *_v,
                  double *_w);

// Begin inline definitions for header-only library

#define UNPACK_POSITION_ARRAY_REF(arr_) (arr_)[X], (arr_)[Y], (arr_)[Z], (arr_)[A], (arr_)[B], (arr_)[C], (arr_)[U], (arr_)[V], (arr_)[W]
#define UNPACK_POSITION_ARRAY(arr_) &(arr_)[X], &(arr_)[Y], &(arr_)[Z], &(arr_)[A], &(arr_)[B], &(arr_)[C], &(arr_)[U], &(arr_)[V], &(arr_)[W]


inline PositionArray toArray(double _x, double _y, double _z, double _a, double _b, double _c, double _u, double _v, double _w)
{
    return {_x, _y, _z, _a, _b, _c, _u, _v, _w};
}

inline PositionArray toArray(const EmcPose & p)
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

inline void extract_axes(PositionArray const &p,
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

inline void extract_axes(const PositionArray & p, double * _x, double * _y, double * _z, double * _a, double * _b, double * _c, double * _u, double * _v, double * _w)
{
    // FIXME no null checks here! be sure you know what you're doing!!
    extract_axes(p, *_x, *_y, *_z, *_a, *_b, *_c, *_u, *_v, *_w);
}

inline PositionArray abs(const PositionArray & p)
{
    PositionArray out;
    for (size_t k=0; k < MAX_USED_AXES; ++k) {
        out[k] = fabs(p[k]);
    }
    return out;
}

inline PositionArray abs_diff(const PositionArray & b, const PositionArray & o)
{
    return abs(b - o);
}

inline double max(const PositionArray & p)
{
    return *std::max_element(p.begin(), p.end());
}

inline double min(const PositionArray & p)
{
    return *std::min_element(p.begin(), p.end());
}

inline EmcPose toEmcPose(const PositionArray & p)
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

inline PositionArray merge(const PositionArray & old_p, const PositionArray & new_p, const AxisMask & new_mask)
{
    PositionArray out{};
    for (size_t k = 0; k < MAX_USED_AXES; ++k) {
        out[k] = new_mask.test(k) ? new_p[k] : old_p[k];
    }
    return out;
}

inline PositionArray toArray(double const *arr_begin, size_t count)
{
    PositionArray out{};
    for (size_t k = 0; k < count; ++k) {
        out[k] = arr_begin[k];
    }
    return out;
}

inline PositionArray toArray(double const *arr_begin, double const *arr_end)
{
    PositionArray out;
    std::copy(arr_begin, arr_end, out.begin());
    return out;
}

inline bool isNonzero(PositionArray const &p)
{
    bool out = false;
    for (size_t k = 0; k < MAX_USED_AXES; ++k) {
        out |= (p[k] != 0.0);
    }
    return out;
}

inline PositionArray max(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        out[i] = std::max(p0[i], p1[i]);
    }
    return out;
}

inline PositionArray min(const PositionArray & p0, const PositionArray & p1)
{
    PositionArray out;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        out[i] = std::min(p0[i], p1[i]);
    }
    return out;
}

inline PositionArray & max_inplace(PositionArray & p0, const PositionArray & p1)
{
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        p0[i] = std::max(p0[i], p1[i]);
    }
    return p0;
}

inline PositionArray & min_inplace(PositionArray & p0, const PositionArray & p1)
{
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        p0[i] = std::min(p0[i], p1[i]);
    }
    return p0;
}

inline char axis_to_char(int ax)
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

inline void apply_abs_mask(PositionArray & p, const PositionArray & min_mask)
{
    for (size_t i=0; i < p.size(); ++i) {
        if (p[i] < min_mask[i]) {
            p[i] = 0;
        }
    }
}

inline double dot(const PositionArray & p0, const PositionArray & p1)
{
    double d = 0;
    for (size_t i=0; i < MAX_USED_AXES; ++i) {
        d += p0[i]*p1[i];
    }
    return d;
}

inline PositionArray &operator*=(PositionArray & p0, double v)
{
    for (auto &p : p0) {
        p*=v;
    }
    return p0;
}

inline PositionArray operator*(PositionArray const & p0, double v)
{
    PositionArray out = p0;
    for (auto &p : out) {
        p*=v;
    }
    return out;
}

inline double mag(const PositionArray & p0)
{
    return sqrt(dot(p0, p0));
}

} //namespace

#endif // POSITIONARRAY_HH
