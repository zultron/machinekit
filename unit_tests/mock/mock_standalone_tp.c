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

void resetMockTP(TP_STRUCT *tp, double cycle_time)
{
    tpClear(tp);
    tpSetCycleTime(tp, cycle_time);
}

TP_STRUCT *newMockTP(unsigned queue_size, double cycle_time)
{
    init_motion_globals();
    TP_STRUCT *tp = malloc(sizeof(TP_STRUCT));
    TC_STRUCT *tcq_space = malloc(sizeof(TC_STRUCT) * queue_size);
    tpCreate(tp, queue_size, tcq_space);
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

