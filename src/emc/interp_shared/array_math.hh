#ifndef ARRAY_MATH_HH
#define ARRAY_MATH_HH
#include <array>

using std::array;

template <typename T, size_t N>
array<T, N> & operator+=(array<T, N> &b, array<T, N> const &o)
{
    for (size_t k=0; k < N; ++k) {
        b[k] += o[k];
    }
    return b;
}

template <typename T, size_t N>
array<T, N> & operator-=(array<T, N> &b, array<T, N> const &o)
{
    for (size_t k=0; k < N; ++k) {
        b[k] -= o[k];
    }
    return b;

}

template <typename T, size_t N>
array<T, N> operator+(array<T, N> const &b, array<T, N> const &o)
{
    auto out = b;
    return out += o;
}

template <typename T, size_t N>
array<T, N> operator-(array<T, N> const &b, array<T, N> const &o)
{
    auto out = b;
    return out -= o;
}

template <typename T, size_t N>
array<T, N> & operator*=(array<T, N> &b, double d)
{
    for (size_t k=0; k < N; ++k) {
        b[k] *= d;
    }
    return b;
}

template <typename T, size_t N>
array<T, N> operator*(array<T, N> const &b, double d)
{
    auto out = b;
    return out *= d;
}

template <typename T, size_t N>
array<T, N> & operator*=(array<T, N> &b, array<T, N> const &other)
{
    for (size_t k=0; k < N; ++k) {
        b[k] *= other[k];
    }
    return b;
}

template <typename T, size_t N>
array<T, N> operator*(array<T, N> const &b, array<T, N> const &other)
{
    auto out = b;
    return out *= other;
}

template <typename T, size_t N>
array<T, N> & operator/=(array<T, N> &b, double d)
{
    for (size_t k=0; k < N; ++k) {
        b[k] /= d;
    }
    return b;
}

template <typename T, size_t N>
array<T, N> operator/(array<T, N> const &b, double d)
{
    auto out = b;
    return out /= d;
}

#endif // ARRAY_MATH_HH
