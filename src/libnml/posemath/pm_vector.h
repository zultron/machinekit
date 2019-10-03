/********************************************************************
* Description: pm_vector.h
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
#ifndef PM_VECTOR_H
#define PM_VECTOR_H
#include "posemath.h"

enum {PM_VECTOR_SIZE=9};

// WARNING this is an array under the hood!
typedef double PmVector [PM_VECTOR_SIZE];

int VecVecCopy(PmVector const v1, PmVector out);
int VecVecAdd(PmVector const v1, PmVector const v2, PmVector out);
int VecVecAddEq(PmVector v1, PmVector const v2);

int VecVecSub(PmVector const  v1, PmVector const v2, PmVector out);
int VecScalMult(PmVector const  v1, double s, PmVector out);
double VecVecDot(PmVector const v1, PmVector const v2);
double VecMag(PmVector const v1);
double VecMagSq(PmVector const v1);
int VecUnit(PmVector const v1, PmVector out);
int VecUnitEq(PmVector v1);
double VecMin(PmVector const v1);
double VecMax(PmVector const v1);

int CartToVec(
    PmCartesian const * const p_xyz,
    PmCartesian const * p_abc,
    PmCartesian const * const puvw,
    PmVector out);
int VecToCart(
    PmVector const vec,
    PmCartesian * p_xyz,
    PmCartesian * p_abc,
    PmCartesian * p_uvw);

int VecVecOrthonormalBasis(
    PmVector const a,
    PmVector const b,
    PmVector u,
    PmVector v);

int VecVecHarmonicAddSq(PmVector const u,
        PmVector const v,
        PmVector scales);

#endif
