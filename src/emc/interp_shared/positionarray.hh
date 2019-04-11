#ifndef POSITIONARRAY_HH
#define POSITIONARRAY_HH

#include <array>
#include <bitset>
struct EmcPose;

static constexpr size_t CANON_AXIS_COUNT = 9;
using PositionArray = std::array<double, CANON_AXIS_COUNT>;

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

using PositionMask = std::bitset<CANON_AXIS_COUNT>;

constexpr std::array<size_t, 6> linearAxes = {X,Y,Z,U,V,W};
constexpr std::array<size_t, 3> angularAxes = {A,B,C};

PositionArray & operator+=(PositionArray &b, PositionArray const &o);
PositionArray & operator-=(PositionArray &b, PositionArray const &o);
PositionArray operator+(PositionArray const &b, PositionArray const &o);
PositionArray operator-(PositionArray const &b, PositionArray const &o);

PositionArray & operator*=(PositionArray &b, double d);
PositionArray operator*(PositionArray const &b, double d);
PositionArray & operator/=(PositionArray &b, double d);
PositionArray operator/(PositionArray const &b, double d);

PositionArray abs(PositionArray const &p);
PositionArray abs_diff(PositionArray const &b, PositionArray const &o);
double max(PositionArray const &p);
double min(PositionArray const & p);

bool isNonzero(PositionArray const &p);

EmcPose toEmcPose(PositionArray const &p);

void merge_masked(PositionArray &p,
                   PositionArray const &in,
                   PositionMask const &new_value_mask);

PositionArray merge(PositionArray const &old_p,
                   PositionArray const &new_p,
                   PositionMask const &new_mask);

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

PositionArray toArray(double *arr_begin,
        size_t count);

PositionArray toArray(double *arr_begin,
        double *arr_end);

PositionArray toArray(EmcPose const &p);

void assign(PositionArray &p,
            double _x,
            double _y,
            double _z,
            double _a,
            double _b,
            double _c,
            double _u,
            double _v,
            double _w);

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

#define UNPACK_POSITION_ARRAY_REF(arr_) (arr_)[X], (arr_)[Y], (arr_)[Z], (arr_)[A], (arr_)[B], (arr_)[C], (arr_)[U], (arr_)[V], (arr_)[W]
#define UNPACK_POSITION_ARRAY(arr_) &(arr_)[X], &(arr_)[Y], &(arr_)[Z], &(arr_)[A], &(arr_)[B], &(arr_)[C], &(arr_)[U], &(arr_)[V], &(arr_)[W]
#endif // POSITIONARRAY_HH
