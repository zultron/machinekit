/********************************************************************
* Description: pm_vector.c
*  Vector structs and functions (generalizes PmCartesian)
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: Robert W. Ellenberg
* License: GPL Version 2
* System: Linux
*
* Copyright (c) 2015 All rights reserved.
********************************************************************/

#include "posemath.h"
#include "pm_vector.h"
#include "rtapi_math.h"

// For internal use only
typedef enum {
    X,
    Y,
    Z,
    A,
    B,
    C,
    U,
    V,
    W,
} PmVectorAxes;

int VecVecCopy(const PmVector v1, PmVector out)
{
    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        out[i] = v1[i];
    }
    return 0;
}

int VecVecAdd(PmVector const v1, PmVector const v2, PmVector out)
{
    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        out[i] = v1[i] + v2[i];
    }
    return 0;
}


int VecVecAddEq(PmVector v1, PmVector const v2)
{
    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        v1[i] += v2[i];
    }
    return 0;
}


int VecVecSub(PmVector const v1, PmVector const v2, PmVector out)
{
    if (!v1 || !v2 || !out) {
        return -1;
    }
    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        out[i] = v1[i] - v2[i];
    }
    return 0;
}

int VecScalMult(PmVector const v1, double s, PmVector out)
{
    if (!v1 || !out) {
        return -1;
    }
    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        out[i] = v1[i]*s;
    }
    return 0;
}

double VecVecDot(PmVector const v1, PmVector const v2)
{
    if (!v1 || !v2) {
        return NAN;
    }
    double dot = 0.0;
    for (int i = 0; i < PM_VECTOR_SIZE; ++i) {
        dot += v1[i] * v2[i];
    }
    return dot;
}

double VecMagSq(PmVector const v1)
{
    return VecVecDot(v1,v1);
}

double VecMag(PmVector const v1)
{
    double mag_sq = VecMagSq(v1);
    return pmSqrt(mag_sq);
}

int VecUnit(PmVector const v1, PmVector out)
{
    return VecVecCopy(v1, out) || VecUnitEq(out);
}

int VecUnitEq(PmVector v1)
{
    double mag = VecMag(v1);
    //FIXME hard-coded cutoff
    if (fabs(mag) < 1e-9)
        return -1;

    VecScalMult(v1, 1.0 / mag, v1);
    return 0;
}

double VecMin(PmVector const v1)
{
    if (!v1) {
        return NAN;
    }

    int i;
    double v_min = v1[0];
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        v_min = fmin(v1[i], v_min);
    }
    return v_min;
}


double VecMax(PmVector const v1)
{
    if (!v1) {
        return NAN;
    }

    int i;
    double v_max = v1[0];
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        v_max = fmax(v1[i], v_max);
    }
    return v_max;
}

int CartToVec(
    PmCartesian const * p_xyz,
    PmCartesian const * p_abc,
    PmCartesian const * p_uvw,
    PmVector out)
{
    if (!p_xyz || !p_uvw || !p_abc || !out) {
        return -1;
    }

    // Copy out elements into our array
    out[X] = p_xyz->x;
    out[Y] = p_xyz->y;
    out[Z] = p_xyz->z;

    out[A] = p_abc->x;
    out[B] = p_abc->y;
    out[C] = p_abc->z;

    out[U] = p_uvw->x;
    out[V] = p_uvw->y;
    out[W] = p_uvw->z;

    return 0;

}

int VecToCart(
    PmVector const vec,
    PmCartesian * p_xyz,
    PmCartesian * p_abc,
    PmCartesian * p_uvw)
{
    if (!vec) {
        return -1;
    }

    if (p_xyz) {
        p_xyz->x = vec[X];
        p_xyz->y = vec[Y];
        p_xyz->z = vec[Z];
    }

    if (p_abc) {
        p_abc->x = vec[A];
        p_abc->y = vec[B];
        p_abc->z = vec[C];
    }

    if (p_uvw) {
        p_uvw->x = vec[U];
        p_uvw->y = vec[V];
        p_uvw->z = vec[W];
    }

    return 0;
}

int VecVecOrthonormalBasis(
    PmVector const a,
    PmVector const b,
    PmVector u,
    PmVector v)
{
    // Find component of b in direction of a
    // NOTE assumes both a and b are already unit
    double dot = VecVecDot(a, b);

    //Build the component in the direction of a
    VecScalMult(b, -dot, v);

    VecVecCopy(a, u);
    // Subtract component of a in direction of b
    VecVecAddEq(v, b);
    VecUnitEq(v);

    return 0;
}


/**
 * Performs "harmonic" addition elementwise on the vector and returns the square of the result.
 * out(i) = arg_max(cos(theta_i) * a(i) + sin(theta_i) * b(i))
 *
 * According to the harmonic addition theorem, this is equivalent to
 * out(i) = arg_max(c(i) * cos(theta_i + d_i)), where d_i is a phase angle, and c(i)^2 = a(i)^2+b(i)^2
 *
 * Therfore, the square of the harmonic addition out(i)^2 = a(i)^2 + b(i)^2.
 *
 * Returning the square term saves 3 square root operations (which can sometimes be avoided depending on the final calculation).
 */
int VecVecHarmonicAddSq(PmVector const u,
                        PmVector const v,
                        PmVector scales)
{
    if (!u || !v || !scales) {
        return -1;
    }

    int i;
    for (i = 0; i < PM_VECTOR_SIZE; ++i) {
        scales[i] = pmSq(u[i]) + pmSq(v[i]);
    }
    return 0;
}

