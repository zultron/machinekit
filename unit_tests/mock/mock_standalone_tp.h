#ifndef MOCK_STANDALONE_TP_H
#define MOCK_STANDALONE_TP_H

#include "tp_types.h"

#define CREATE_MOCK_TP() TP_STRUCT *tp = newMockTP(1000, 0.001)

TP_STRUCT *newMockTP(unsigned queue_size, double cycle_time);
void deleteMockTP(TP_STRUCT **tp);
TP_STRUCT *getMockTP();

#endif // MOCK_STANDALONE_TP_H
