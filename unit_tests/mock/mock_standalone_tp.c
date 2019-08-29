/**
 * @file mock_standalone_tp.c
 * Mocks for testing TP API without dependence on motion.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#include "mock_standalone_tp.h"
#include "tp.h"
#include "tcq.h"
#include "stdlib.h"
#include "mot_priv.h"
#include "mock_motion_global.h"
#include "string.h"

TC_STRUCT tcq_space[1000];

void resetMockTP(TP_STRUCT *tp, double cycle_time)
{
    tpClear(tp);
    tpSetCycleTime(tp, cycle_time);
}

TP_STRUCT *newMockTP(double cycle_time, double a_max)
{
    init_motion_globals(a_max);
    TP_STRUCT *tp = malloc(sizeof(TP_STRUCT));
    memset(tcq_space, 0, sizeof(tcq_space));
    tpCreate(tp, 1000, tcq_space);
    resetMockTP(tp, cycle_time);
    return tp;
}

void deleteMockTP(TP_STRUCT ** tp)
{
    if (tp && *tp) {
        tcqDelete(&(*tp)->queue);
        free(*tp);
        *tp=NULL;
    }
}

