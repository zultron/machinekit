#ifndef TP_PRIV_H
#define TP_PRIV_H

#include "tp_enums.h"
#include "tc_types.h"

struct TP_STRUCT;

typedef struct {
    char axes[9];
} AxisMaskString;

int tpComputeBlendVelocity(
        TC_STRUCT const *tc,
        TC_STRUCT const *nexttc,
        double v_target_this,
        double v_target_next,
        double *v_blend_this,
        double *v_blend_next,
        double *v_blend_net);

double estimateParabolicBlendPerformance(
        TP_STRUCT const *tp,
        TC_STRUCT const *tc,
        TC_STRUCT const *nexttc);

int tpCheckEndCondition(TP_STRUCT const * const tp,
                        TC_STRUCT * const tc,
                        TC_STRUCT const * const nexttc);

int tpUpdateCycle(TP_STRUCT * const tp,
        TC_STRUCT * const tc,
        TC_STRUCT const * const nexttc,
        UpdateCycleMode cycle_mode);

int tpRunOptimization(TP_STRUCT * const tp);

int tpAddSegmentToQueue(TP_STRUCT * const tp, TC_STRUCT * const tc, int inc_id);

double tpGetMaxTargetVel(TP_STRUCT const * const tp, TC_STRUCT const * const tc);

int tpIsWaitingOnSpindle(TP_STRUCT const * const tp);

int tpHandleBlendArc(TP_STRUCT * const tp, TC_STRUCT * const tc);

tp_err_t tpCreateBiarcBlend(TP_STRUCT * const tp, TC_STRUCT * const prev_tc, TC_STRUCT * const this_tc);

int tcRotaryMotionCheck(TC_STRUCT const * const tc);

double tpGetTangentKinkRatio(void);

double tpGetFeedScale(TP_STRUCT const * const tp,
                      TC_STRUCT const * const tc);

double tpGetRealTargetVel(TP_STRUCT const * const tp,
                          TC_STRUCT const * const tc);

double getMaxFeedScale(TC_STRUCT const * tc);

double getMaxBlendFeedScale(TC_STRUCT const * prev_tc, TC_STRUCT const * tc);

double tpGetMaxTargetVel(TP_STRUCT const * const tp, TC_STRUCT const * const tc);

double tpGetRealFinalVel(TP_STRUCT const * const tp,
                         TC_STRUCT const * const tc, TC_STRUCT const * const nexttc);

void resetSpindleOrigin(spindle_origin_t *origin);

void setSpindleOrigin(spindle_origin_t *origin, double position);

double tpCalculateTriangleVel(TC_STRUCT const *tc);

double tpCalculateOptimizationInitialVel(TP_STRUCT const * const tp, TC_STRUCT * const tc);

tc_blend_type_t tpChooseBestBlend(TP_STRUCT const * const tp,
                                  TC_STRUCT * const prev_tc,
                                  TC_STRUCT * const tc,
                                  TC_STRUCT * const blend_tc);

tp_err_t tpCreateLineArcBlend(TP_STRUCT * const tp,
                              TC_STRUCT * const prev_tc,
                              TC_STRUCT * const tc,
                              TC_STRUCT * const blend_tc);

tp_err_t tpCreateArcLineBlend(TP_STRUCT * const tp,
                              TC_STRUCT * const prev_tc,
                              TC_STRUCT * const tc,
                              TC_STRUCT * const blend_tc);

tp_err_t tpCreateArcArcBlend(TP_STRUCT * const tp,
                             TC_STRUCT * const prev_tc,
                             TC_STRUCT * const tc,
                             TC_STRUCT * const blend_tc);

tp_err_t tpCreateLineLineBlend(TP_STRUCT * const tp,
                               TC_STRUCT * const prev_tc,
                               TC_STRUCT * const tc,
                               TC_STRUCT * const blend_tc);

int handlePrevTermCondition(TC_STRUCT *prev_tc, TC_STRUCT *tc);
int handleModeChange(TC_STRUCT *prev_tc, TC_STRUCT *tc);
int tpSetupSyncedIO(TP_STRUCT * const tp, TC_STRUCT * const tc);
int tcUpdateArcLengthFit(TC_STRUCT * const tc);
int tpFinalizeAndEnqueue(TP_STRUCT * const tp, TC_STRUCT * const tc);

blend_type_t tpCheckBlendArcType(TP_STRUCT const * const tp,
                                 TC_STRUCT const * const prev_tc,
                                 TC_STRUCT const * const tc);

int tpComputeOptimalVelocity(
    TP_STRUCT const * const tp,
    TC_STRUCT * const tc,
    TC_STRUCT * const prev1_tc);

TCIntersectType tpSetupTangent(
    TP_STRUCT const * const tp,
    TC_STRUCT * const prev_tc,
    TC_STRUCT * const tc);

int tcUpdateDistFromAccel(TC_STRUCT * const tc, double acc, double vel_desired);

void tpDebugCycleInfo(TP_STRUCT const * const tp,
                      TC_STRUCT const * const tc,
                      TC_STRUCT const * const nexttc,
                      double acc,
                      int accel_mode,
                      UpdateCycleMode cycle);

int tpCalculateRampAccel(TP_STRUCT const * const tp,
                         TC_STRUCT * const tc,
                         TC_STRUCT const * const nexttc,
                         double * const acc,
                         double * const vel_desired);

void tpUpdateRigidTapState(TP_STRUCT * const tp,
                           TC_STRUCT * const tc);

int tpUpdateMovementStatus(TP_STRUCT * const tp,
                           TC_STRUCT const * const tc,
                           TC_STRUCT const * const nexttc);

void tpUpdateBlend(TP_STRUCT * const tp,
                   TC_STRUCT * const tc,
                   TC_STRUCT * const nexttc);

void tpHandleEmptyQueue(TP_STRUCT * const tp);

void tpSetRotaryUnlock(int axis, int unlock);

int tpGetRotaryIsUnlocked(int axis);

int tpCompleteSegment(TP_STRUCT * const tp,
                      TC_STRUCT const * const tc);

tp_err_t tpHandleAbort(TP_STRUCT * const tp,
                       TC_STRUCT * const tc,
                       TC_STRUCT * const nexttc);

tp_err_t tpCheckAtSpeed(TP_STRUCT * const tp, TC_STRUCT * const tc);

tp_err_t tpActivateSegment(TP_STRUCT * const tp, TC_STRUCT * const tc);

void tpSyncVelocityMode(TC_STRUCT * const tc, TC_STRUCT * const nexttc);

void checkPositionSyncError(TP_STRUCT const *tp, TC_STRUCT const *tc);

void clearPositionSyncErrors();
void clearPosTrackingStatus();

double findSpindleDisplacement(
        double new_pos,
        spindle_origin_t origin
    );

double findSpindleVelocity(
        double spindle_velocity,
        spindle_origin_t origin
    );

bool spindleReversed(spindle_origin_t origin, double prev_pos, double current_pos);

bool cmdReverseSpindle(void);

AxisMaskString axisBitMaskToString(unsigned failed_axes);

void reportTPAxisError(TP_STRUCT const *tp, unsigned failed_axes, const char *msg_prefix);


void tpSyncPositionMode(TP_STRUCT * const tp,
                        TC_STRUCT * const tc,
                        TC_STRUCT * const nexttc );

int tpDoParabolicBlending(TP_STRUCT * const tp,
                          TC_STRUCT * const tc,
                          TC_STRUCT * const nexttc);

int tpUpdateInitialStatus(TP_STRUCT const * const tp);

int tcSetSplitCycle(TC_STRUCT * const tc,
                    double split_time,
                    double v_f);

int tpHandleRegularCycle(TP_STRUCT * const tp,
                         TC_STRUCT * const tc,
                         TC_STRUCT * const nexttc);

int tpHandleSplitCycle(TP_STRUCT * const tp,
                       TC_STRUCT * const tc,
                       TC_STRUCT * const nexttc);

int tpSteppingCheck(TP_STRUCT * const tp,
                    TC_STRUCT * const tc,
                    TC_STRUCT * const nexttc);

int tpInitBlendArcFromPrev(TP_STRUCT const * const tp,
                           TC_STRUCT const * const prev_tc,
                           TC_STRUCT * const blend_tc,
                           double vel,
                           double ini_maxvel,
                           double acc,
                           tc_motion_type_t motion_type);

int tcSetLineXYZ(TC_STRUCT * const tc, PmCartLine const * const line);

int find_max_element(double arr[], int sz);

#endif // TP_PRIV_H
