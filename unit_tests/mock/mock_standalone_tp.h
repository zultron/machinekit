#ifndef MOCK_STANDALONE_TP_H
#define MOCK_STANDALONE_TP_H

#include "tp_types.h"

TP_STRUCT *newMockTP(unsigned queue_size, double cycle_time);
void deleteMockTP(TP_STRUCT **tp);
TP_STRUCT *getMockTP();

#endif // MOCK_STANDALONE_TP_H
