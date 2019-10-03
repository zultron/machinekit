/**
 * @file pm_line9.c
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#include "posemath.h"
#include "pm_line9.h"
#include "rtapi_bool.h"
#include "stddef.h"
#include "rtapi_math.h"

static const PmCartesian zero={0,0,0};

int pmLine9Init(
    PmLine9 * const line9,
    PmVector const * const start,
    PmVector const * const end,
    bool ignore_xyz)
{
    line9->start = *start;
    line9->end = *end;

    if (ignore_xyz) {
        VecSetXYZ(&zero, &line9->start);
        VecSetXYZ(&zero, &line9->end);
    }

    VecVecSub(&line9->end, &line9->start, &line9->uVec);
    line9->tmag = VecMag(&line9->uVec);
    return 0;
}

double pmLine9Length(PmLine9 const * const line9)
{
    return line9->tmag;
}

double pmLine9RotaryAxisLength(PmLine9 const * const line9)
{
    PmCartesian abc;
    VecToCart(&line9->end, NULL, &abc, NULL);
    PmCartesian abc_start;
    VecToCart(&line9->start, NULL, &abc_start, NULL);

    pmCartCartSubEq(&abc, &abc_start);
    double len=NAN;
    pmCartMag(&abc, &len);
    return len;
}

double pmLine9LinearAxisLength(const PmLine9 * const line9)
{
    PmVector no_abc;
    VecVecSub(&line9->end, &line9->start, &no_abc);
    VecSetABC(&zero, &no_abc);

    return VecMag(&no_abc);
}

int pmLine9Point(const PmLine9 * const line9, double progress, PmVector * const point)
{
    return
        VecScalMult(&line9->uVec, progress, point)
        ||    VecVecAddEq(point, &line9->start);
}

int pmLine9Stretch(PmLine9 * const line9, double new_len, int from_end)
{
    // Use short-circuit evaluation to prevent error propagation
    if (from_end) {
        return
            VecScalMult(&line9->uVec, -new_len, &line9->start)
            || VecVecAddEq(&line9->start, &line9->end);
    } else {
        return
            VecScalMult(&line9->uVec, new_len, &line9->end)
            || VecVecAddEq(&line9->end, &line9->start);
    }
}

