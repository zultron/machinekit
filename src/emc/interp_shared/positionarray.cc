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
    PositionArray out;
    assign(out, _x, _y, _z, _a, _b, _c, _u, _v, _w);
    return out;
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

void assign(PositionArray & p, double _x, double _y, double _z, double _a, double _b, double _c, double _u, double _v, double _w)
{
    p[X] = _x;
    p[Y] = _y;
    p[Z] = _z;
    p[A] = _a;
    p[B] = _b;
    p[C] = _c;
    p[U] = _u;
    p[V] = _v;
    p[W] = _w;
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

void merge_masked(PositionArray & p, const PositionArray & in, const PositionMask & new_value_mask)
{
    for (size_t k = 0; k < in.size(); ++k) {
        if (new_value_mask[k]) {
            p[k] = in[k];
        }
    }
}

PositionArray merge(const PositionArray & old_p, const PositionArray & new_p, const PositionMask & new_mask)
{
    PositionArray out = old_p;
    merge_masked(out, new_p, new_mask);
    return out;
}

PositionArray toArray(double *arr_begin, size_t count)
{
    PositionArray out{};
    for (size_t k = 0; k < count; ++k) {
        out[k] = arr_begin[k];
    }
    return out;
}

PositionArray toArray(double *arr_begin, double *arr_end)
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
