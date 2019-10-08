#ifndef PM_LINE9_H
#define PM_LINE9_H

#include "pm_vector.h"
#include "rtapi_bool.h"

typedef struct {
    PmVector start;
    PmVector end;
    PmVector uVec;
    double tmag;
} PmLine9;

int pmLine9Init(PmLine9 * const line9,
    PmVector const * const start,
    PmVector const * const end,
                bool ignore_xyz);

double pmLine9Length(const PmLine9 * const line9);

double pmLine9RotaryAxisLength(PmLine9 const * const line9);

double pmLine9LinearAxisLength(PmLine9 const * const line9);

int pmLine9Point(PmLine9 const * const line9, double progress, PmVector * const point);

int pmLine9Stretch(PmLine9 * const line, double new_len, int from_end);

#endif // PM_LINE9_H
