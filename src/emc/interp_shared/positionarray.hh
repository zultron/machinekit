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

#include <array>
#include <bitset>
#include <array_math.hh>

struct EmcPose;

static constexpr const size_t CANON_AXIS_COUNT = 9;
using PositionArray = std::array<double, CANON_AXIS_COUNT>;

// For now identical to the total count of axes but may be smaller for special applications
static constexpr const size_t MAX_USED_AXES = CANON_AXIS_COUNT;

enum PositionArrayIndex
{
    X,
    Y,
    Z,
    A,
    B,
    C,
    U,
    V,
    W,
};

static constexpr std::array<char, CANON_AXIS_COUNT> AxisNames = {
    'X',
    'Y',
    'Z',
    'A',
    'B',
    'C',
    'U',
    'V',
    'W',
};

char axis_to_char(int ax);

using AxisMask = std::bitset<64>;

const constexpr std::array<size_t, 6> linearAxes = {X,Y,Z,U,V,W};
const constexpr std::array<size_t, 3> angularAxes = {A,B,C};
const constexpr AxisMask angularAxisMask = 1 << A |
                                     1 << B |
                                     1 << C;
const constexpr AxisMask linearAxisMask = 1 << X |
                                    1 << Y |
                                    1 << Z |
                                    1 << U |
                                    1 << V |
                                    1 << W;

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

#define UNPACK_POSITION_ARRAY_REF(arr_) (arr_)[X], (arr_)[Y], (arr_)[Z], (arr_)[A], (arr_)[B], (arr_)[C], (arr_)[U], (arr_)[V], (arr_)[W]
#define UNPACK_POSITION_ARRAY(arr_) &(arr_)[X], &(arr_)[Y], &(arr_)[Z], &(arr_)[A], &(arr_)[B], &(arr_)[C], &(arr_)[U], &(arr_)[V], &(arr_)[W]

#endif // POSITIONARRAY_HH
