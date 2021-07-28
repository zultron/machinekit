/********************************************************************
* Description: interp_queue.hh
*
* Author: Chris Radek
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2009 All rights reserved.
*
********************************************************************/

#ifndef INTERP_QUEUE_HH
#define INTERP_QUEUE_HH

#include <interp_internal.hh>
#include <vector>
#include "StateTag.hh"

struct StraightTraverse {
    int line_number;
    double dx, dy, dz;          // direction of original motion
    double x,y,z, a,b,c, u,v,w;
};

struct StraightFeed {
    int line_number;
    double dx, dy, dz;          // direction of original motion
    double x,y,z, a,b,c, u,v,w; // target
};

struct ArcFeed {
    int line_number;
    double original_turns;
    bool allow_implicit_full_turn;
    double end1, end2, center1, center2;
    int turn;
    double end3, a,b,c, u,v,w;
};

struct SetFeedRate {
    double feed;
};

struct Dwell {
    double time;
};

struct SetFeedMode {
    int mode;
};

struct SetSpindleMode {
    double mode;
};

struct SetSpindleSpeed {
    double speed;
};

struct Comment {
    char *comment;
};

struct MUserCommand {
    int    index;
    double p_number;
    double q_number;
};

struct OrientSpindle {
    double orientation;
    int mode;
};

struct WaitOrientSpindleComplete {
    double timeout;
};

enum QCEnum {
    QINVALID,
    QSTRAIGHT_TRAVERSE,
    QSTRAIGHT_FEED,
    QARC_FEED,
    QSET_FEED_RATE,
    QDWELL,
    QSET_FEED_MODE,
    QMIST_ON,
    QMIST_OFF,
    QFLOOD_ON,
    QFLOOD_OFF,
    QSTART_SPINDLE_CLOCKWISE,
    QSTART_SPINDLE_COUNTERCLOCKWISE,
    QSTOP_SPINDLE_TURNING,
    QSET_SPINDLE_MODE,
    QSET_SPINDLE_SPEED,
    QCOMMENT,
    QM_USER_COMMAND,
    QSTART_CHANGE,
    QORIENT_SPINDLE,
    QWAIT_ORIENT_SPINDLE_COMPLETE
};

union QueuedCanonUnion {
    StraightTraverse straight_traverse;
    StraightFeed straight_feed;
    ArcFeed arc_feed;
    SetFeedRate set_feed_rate;
    Dwell dwell;
    SetFeedMode set_feed_mode;
    SetSpindleMode set_spindle_mode;
    SetSpindleSpeed set_spindle_speed;
    Comment comment;
    MUserCommand mcommand;
    OrientSpindle orient_spindle;
    WaitOrientSpindleComplete wait_orient_spindle_complete;
};

/**
 * Map from a given Queued Canon proper type to the enum that will represent it in the tagged union.
 * Note that not all enums have a corresponding data type (e.g. mist / flood) because those messages carry no state.
 */
template<typename QueuedCanonTypeName>
struct QCTypeToEnum;

template<QCEnum tagged_type>
struct QCEnumToType;

/** Map from QC types to enum value and back using template specialization.
 * Macros are ugly but at least avoid repetitions in the declaration. A nice side effect
 * of this double-declaration in the macro is that duplicate enums throw an error.
 */
#define QC_TYPE_ENUM_DECL(t, e) \
template<> struct QCTypeToEnum<t> { static constexpr QCEnum qc_enum = e; }; \
template<> struct QCEnumToType<e> { using QCType = t; } \

// Declare individual specializations of the mapping between tagged type and member type
QC_TYPE_ENUM_DECL(StraightTraverse, QCEnum::QSTRAIGHT_TRAVERSE);
QC_TYPE_ENUM_DECL(StraightFeed, QCEnum::QSTRAIGHT_FEED);
QC_TYPE_ENUM_DECL(ArcFeed, QCEnum::QARC_FEED);
QC_TYPE_ENUM_DECL(SetFeedRate, QCEnum::QSET_FEED_RATE);
QC_TYPE_ENUM_DECL(Dwell, QCEnum::QDWELL);
QC_TYPE_ENUM_DECL(SetFeedMode, QCEnum::QSET_FEED_MODE);
QC_TYPE_ENUM_DECL(SetSpindleMode, QCEnum::QSET_SPINDLE_MODE);
QC_TYPE_ENUM_DECL(SetSpindleSpeed, QCEnum::QSET_SPINDLE_SPEED);
QC_TYPE_ENUM_DECL(Comment, QCEnum::QCOMMENT);
QC_TYPE_ENUM_DECL(MUserCommand, QCEnum::QM_USER_COMMAND);
QC_TYPE_ENUM_DECL(OrientSpindle, QCEnum::QORIENT_SPINDLE);
QC_TYPE_ENUM_DECL(WaitOrientSpindleComplete, QCEnum::QWAIT_ORIENT_SPINDLE_COMPLETE);

#undef QC_TYPE_ENUM_DECL

template <typename T>
T& qcUnionMember(QueuedCanonUnion& u)
{
    return *((T*)&u);
}

struct QueuedCanon {
    QueuedCanon() = default;
    QueuedCanon(QCEnum qc_type, StateTag const &t);

    template <typename QueuedCanonTypeName>
    QueuedCanon(QueuedCanonTypeName const &f, StateTag const &t) :
        qc_type(QCTypeToEnum<QueuedCanonTypeName>::qc_enum),
        tag(t)
    {
        qcUnionMember<QueuedCanonTypeName>(data) = f;
    }

    QCEnum qc_type;
    QueuedCanonUnion data;

    StateTag tag;
};

std::vector<QueuedCanon>& qc(void);

void enqueue_SET_FEED_RATE(double feed, StateTag const &tag);
void enqueue_DWELL(double time, StateTag const &tag);
void enqueue_SET_FEED_MODE(int mode, StateTag const &tag);
void enqueue_MIST_ON(StateTag const &tag);
void enqueue_MIST_OFF(StateTag const &tag);
void enqueue_FLOOD_ON(StateTag const &tag);
void enqueue_FLOOD_OFF(StateTag const &tag);
void enqueue_START_SPINDLE_CLOCKWISE(StateTag const &tag);
void enqueue_START_SPINDLE_COUNTERCLOCKWISE(StateTag const &tag);
void enqueue_STOP_SPINDLE_TURNING(StateTag const &tag);
void enqueue_SET_SPINDLE_MODE(double mode, StateTag const &tag);
void enqueue_SET_SPINDLE_SPEED(double speed, StateTag const &tag);
void enqueue_COMMENT(const char *c);
int enqueue_STRAIGHT_FEED(CANON_PLANE plane, int l,
                          double dx, double dy, double dz,
                          double x, double y, double z,
                          double a, double b, double c,
                          double u, double v, double w, StateTag const &tag);
int enqueue_STRAIGHT_TRAVERSE(CANON_PLANE plane, int l,
                              double dx, double dy, double dz,
                              double x, double y, double z,
                              double a, double b, double c,
                              double u, double v, double w, StateTag const &tag);
void enqueue_ARC_FEED(int l,
                      double original_arclen,
                      bool allow_implicit_full_turn,
                      double end1, double end2, double center1, double center2,
                      int turn,
                      double end3,
                      double a, double b, double c,
                      double u, double v, double w, StateTag const &tag);
void enqueue_M_USER_COMMAND(int index,double p_number,double q_number, StateTag const &tag);
void enqueue_START_CHANGE(StateTag const &tag);
void enqueue_ORIENT_SPINDLE(double orientation, int mode, StateTag const &tag);
void enqueue_WAIT_ORIENT_SPINDLE_COMPLETE(double timeout, StateTag const &tag);
void dequeue_canons(setup_pointer settings);
void set_endpoint(double x, double y);
void set_endpoint_zx(double z, double x);
int move_endpoint_and_flush(setup_pointer settings, double x, double y);
void qc_reset(void);
void qc_scale(double scale);

#endif
