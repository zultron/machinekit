#ifndef MOCK_STANDALONE_TP_H
#define MOCK_STANDALONE_TP_H

#include "tp_types.h"

#define CREATE_MOCK_TP_ACC(_a_max) TP_STRUCT *tp = newMockTP(0.001, _a_max)
#define CREATE_MOCK_TP() TP_STRUCT *tp = newMockTP(0.001, 20.0)

void resetMockTP(TP_STRUCT * tp, double cycle_time);
TP_STRUCT *newMockTP(double cycle_time, double a_max);
void deleteMockTP(TP_STRUCT **tp);
TP_STRUCT *getMockTP();
struct state_tag_t line_tag(int line_no);

EmcPose newXYZ(double x, double y, double z, EmcPose last);
#endif // MOCK_STANDALONE_TP_H
