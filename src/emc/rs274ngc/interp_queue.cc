/********************************************************************
* Description: interp_queue.cc
*
* Author: Chris Radek
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2009 All rights reserved.
*
********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "rs274ngc.hh"
#include "rs274ngc_return.hh"
#include "interp_queue.hh"
#include "interp_internal.hh"
#include "rs274ngc_interp.hh"
#include "arc_feed_util.hh"

QueuedCanon::QueuedCanon(QCEnum type_in, const StateTag & t) :
    qc_type(type_in),
    tag(t)
{
}


static int debug_qc = 0;

// lathe tools have strange origin points that are not at
// the center of the radius.  This means that the point that
// radius compensation controls (center of radius) is not at
// the tool's origin.  These functions do the necessary
// translation.  Notice tool orientations 0 (mill) and 9, and 
// those with radius 0 (a point) do not need any translation.

static double latheorigin_x(setup_pointer settings, double x) {
    int o = settings->cutter_comp.orientation;
    double r = settings->cutter_comp.radius;
    if(settings->plane != CANON_PLANE_XZ) return x;

    if(o==2 || o==6 || o==1) x -= r;
    if(o==3 || o==8 || o==4) x += r;
    return x;
}

static double latheorigin_z(setup_pointer settings, double z) {
    int o = settings->cutter_comp.orientation;
    double r = settings->cutter_comp.radius;
    if(settings->plane != CANON_PLANE_XZ) return z;

    if(o==2 || o==7 || o==3) z -= r;
    if(o==1 || o==5 || o==4) z += r;
    return z;
}

static double endpoint[2];
static int endpoint_valid = 0;

std::vector<QueuedCanon>& qc(void) {
    static std::vector<QueuedCanon> c;
#if 0
    printf("len %d\n", (int)c.size());
#endif
    return c;
}

void qc_reset(void) {
    if(debug_qc) printf("qc cleared\n");
    qc().clear();
    endpoint_valid = 0;
}

void enqueue_SET_FEED_RATE(double feed, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate set feed rate %f\n", feed);
        UPDATE_TAG(tag);
        SET_FEED_RATE(feed);
        return;
    }
    if(debug_qc) printf("enqueue set feed rate %f\n", feed);
    qc().emplace_back(SetFeedRate{feed}, tag);
}

void enqueue_DWELL(double time, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate dwell %f\n", time);
        UPDATE_TAG(tag);
        DWELL(time);
        return;
    }
    if(debug_qc) printf("enqueue dwell %f\n", time);
    qc().emplace_back(Dwell{time}, tag);
}

void enqueue_SET_FEED_MODE(int mode, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate set feed mode %d\n", mode);
        UPDATE_TAG(tag);
        SET_FEED_MODE(mode);
        return;
    }
    if(debug_qc) printf("enqueue set feed mode %d\n", mode);
    qc().emplace_back(SetFeedMode{mode}, tag);
}

void enqueue_MIST_ON(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate mist on\n");
        UPDATE_TAG(tag);
        MIST_ON();
        return;
    }
    if(debug_qc) printf("enqueue mist on\n");
    qc().emplace_back(QMIST_ON, tag);
}

void enqueue_MIST_OFF(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate mist off\n");
        UPDATE_TAG(tag);
        MIST_OFF();
        return;
    }
    if(debug_qc) printf("enqueue mist off\n");
    qc().emplace_back(QMIST_OFF, tag);
}

void enqueue_FLOOD_ON(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate flood on\n");
        UPDATE_TAG(tag);
        FLOOD_ON();
        return;
    }
    if(debug_qc) printf("enqueue flood on\n");
    qc().emplace_back(QFLOOD_ON, tag);
}

void enqueue_FLOOD_OFF(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate flood on\n");
        UPDATE_TAG(tag);
        FLOOD_OFF();
        return;
    }
    if(debug_qc) printf("enqueue flood off\n");
    qc().emplace_back(QFLOOD_OFF, tag);
}

void enqueue_START_SPINDLE_CLOCKWISE(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate spindle clockwise\n");
        UPDATE_TAG(tag);
        START_SPINDLE_CLOCKWISE();
        return;
    }
    if(debug_qc) printf("enqueue spindle clockwise\n");
    qc().emplace_back(QSTART_SPINDLE_CLOCKWISE, tag);
}

void enqueue_START_SPINDLE_COUNTERCLOCKWISE(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate spindle counterclockwise\n");
        UPDATE_TAG(tag);
        START_SPINDLE_COUNTERCLOCKWISE();
        return;
    }
    if(debug_qc) printf("enqueue spindle counterclockwise\n");
    qc().emplace_back(QSTART_SPINDLE_COUNTERCLOCKWISE, tag);
}

void enqueue_STOP_SPINDLE_TURNING(StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate spindle stop\n");
        UPDATE_TAG(tag);
        STOP_SPINDLE_TURNING();
        return;
    }
    if(debug_qc) printf("enqueue spindle stop\n");
    qc().emplace_back(QSTOP_SPINDLE_TURNING, tag);
}

void enqueue_ORIENT_SPINDLE(double orientation, int mode, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate spindle orient\n");
        UPDATE_TAG(tag);
        ORIENT_SPINDLE(orientation,mode);
        return;
    }
    if(debug_qc) printf("enqueue spindle orient\n");
    qc().emplace_back(OrientSpindle{orientation, mode}, tag);
}

void enqueue_WAIT_ORIENT_SPINDLE_COMPLETE(double timeout, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate wait spindle orient complete\n");
        UPDATE_TAG(tag);
        WAIT_SPINDLE_ORIENT_COMPLETE(timeout);
        return;
    }
    if(debug_qc) printf("enqueue wait spindle orient complete\n");
    qc().emplace_back(WaitOrientSpindleComplete{timeout}, tag);
}

void enqueue_SET_SPINDLE_MODE(double mode, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate spindle mode %f\n", mode);
        UPDATE_TAG(tag);
        SET_SPINDLE_MODE(mode);
        return;
    }
    if(debug_qc) printf("enqueue spindle mode %f\n", mode);
    qc().emplace_back(SetSpindleMode{mode}, tag);
}

void enqueue_SET_SPINDLE_SPEED(double speed, StateTag const &tag) {
    if(qc().empty()) {
    if(debug_qc) printf("immediate set spindle speed %f\n", speed);
        UPDATE_TAG(tag);
        SET_SPINDLE_SPEED(speed);
        return;
    }
    if(debug_qc) printf("enqueue set spindle speed %f\n", speed);
    qc().emplace_back(SetSpindleSpeed{speed}, tag);
}

void enqueue_COMMENT(const char *c) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate comment \"%s\"\n", c);
        COMMENT(c);
        return;
    }
    Comment comment{strdup(c)};
    if(debug_qc) printf("enqueue comment \"%s\"\n", c);
    qc().emplace_back(comment, StateTag());
}

int enqueue_STRAIGHT_FEED(CANON_PLANE plane, int l,
                           double dx, double dy, double dz,
                           double x, double y, double z, 
                           double a, double b, double c,
                           double u, double v, double w, StateTag const &tag) {
    StraightFeed straight_feed;
    straight_feed.line_number = l;
    switch(plane) {
    case CANON_PLANE_XY:
        straight_feed.dx = dx;
        straight_feed.dy = dy;
        straight_feed.dz = dz;
        straight_feed.x = x;
        straight_feed.y = y;
        straight_feed.z = z;
        break;
    case CANON_PLANE_XZ:
        straight_feed.dz = dx;
        straight_feed.dx = dy;
        straight_feed.dy = dz;
        straight_feed.z = x;
        straight_feed.x = y;
        straight_feed.y = z;
        break;
    default:
        ;
    }        
    straight_feed.a = a;
    straight_feed.b = b;
    straight_feed.c = c;
    straight_feed.u = u;
    straight_feed.v = v;
    straight_feed.w = w;
    qc().emplace_back(straight_feed, tag);
    if(debug_qc) printf("enqueue straight feed lineno %d to %f %f %f direction %f %f %f\n", l, x,y,z, dx, dy, dz);
    return 0;
}

int enqueue_STRAIGHT_TRAVERSE(CANON_PLANE plane, int l,
                               double dx, double dy, double dz,
                               double x, double y, double z, 
                               double a, double b, double c, 
                               double u, double v, double w, StateTag const &tag) {
    StraightTraverse traverse;
    traverse.line_number = l;
    switch(plane) {
    case CANON_PLANE_XY:
        traverse.dx = dx;
        traverse.dy = dy;
        traverse.dz = dz;
        traverse.x = x;
        traverse.y = y;
        traverse.z = z;
        break;
    case CANON_PLANE_XZ:
        traverse.dz = dx;
        traverse.dx = dy;
        traverse.dy = dz;
        traverse.z = x;
        traverse.x = y;
        traverse.y = z;
        break;
    default:
        ;
    }        
    traverse.a = a;
    traverse.b = b;
    traverse.c = c;
    traverse.u = u;
    traverse.v = v;
    traverse.w = w;
    if(debug_qc) printf("enqueue straight traverse lineno %d to %f %f %f direction %f %f %f\n", l, x,y,z, dx, dy, dz);
    qc().emplace_back(traverse, tag);
    return 0;
}

void enqueue_ARC_FEED(int l,
                      double original_turns,
                      bool allow_implicit_full_turn,
                      double end1, double end2, double center1, double center2,
                      int turn,
                      double end3,
                      double a, double b, double c,
                      double u, double v, double w, StateTag const &tag) {
    ArcFeed arc_feed;
    arc_feed.line_number = l;
    arc_feed.original_turns = original_turns;
    arc_feed.allow_implicit_full_turn = allow_implicit_full_turn;
    arc_feed.end1 = end1;
    arc_feed.end2 = end2;
    arc_feed.center1 = center1;
    arc_feed.center2 = center2;
    arc_feed.turn = turn;
    arc_feed.end3 = end3;
    arc_feed.a = a;
    arc_feed.b = b;
    arc_feed.c = c;
    arc_feed.u = u;
    arc_feed.v = v;
    arc_feed.w = w;

    if(debug_qc) printf("enqueue arc lineno %d to %f %f center %f %f turn %d sweeping %f\n", l, end1, end2, center1, center2, turn, original_turns);
    qc().emplace_back(arc_feed, tag);
}

void enqueue_M_USER_COMMAND (int index, double p_number, double q_number, StateTag const &tag) {
    if(qc().empty()) {
        if(debug_qc) printf("immediate M_USER_COMMAND index=%d p=%f q=%f\n",
                           index,p_number,q_number);
        (*(USER_DEFINED_FUNCTION[index - 100])) (index - 100,p_number,q_number);
        return;
    }
    MUserCommand mcommand;
    mcommand.index    = index;
    mcommand.p_number = p_number;
    mcommand.q_number = q_number;

    if(debug_qc) printf("enqueue M_USER_COMMAND index=%d p=%f q=%f\n",
                        index,p_number,q_number);
    qc().emplace_back(mcommand, tag);
}

void enqueue_START_CHANGE (StateTag const &tag) {
    if(debug_qc) printf("enqueue START_CHANGE\n");
    qc().emplace_back(QSTART_CHANGE, tag);
}


void qc_scale(double scale) {
    
    if(qc().empty()) {
        if(debug_qc) printf("not scaling because qc is empty\n");
        return;
    }

    if(debug_qc) printf("scaling qc by %f\n", scale);

    for(auto &q : qc()) {
        endpoint[0] *= scale;
        endpoint[1] *= scale;
        switch(q.qc_type) {
        case QARC_FEED:
            q.data.arc_feed.end1 *= scale;
            q.data.arc_feed.end2 *= scale;
            q.data.arc_feed.end3 *= scale;
            q.data.arc_feed.center1 *= scale;
            q.data.arc_feed.center2 *= scale;
            q.data.arc_feed.u *= scale;
            q.data.arc_feed.v *= scale;
            q.data.arc_feed.w *= scale;
            break;
        case QSTRAIGHT_FEED:
            q.data.straight_feed.x *= scale;
            q.data.straight_feed.y *= scale;
            q.data.straight_feed.z *= scale;
            q.data.straight_feed.u *= scale;
            q.data.straight_feed.v *= scale;
            q.data.straight_feed.w *= scale;
            break;
        case QSTRAIGHT_TRAVERSE:
            q.data.straight_traverse.x *= scale;
            q.data.straight_traverse.y *= scale;
            q.data.straight_traverse.z *= scale;
            q.data.straight_traverse.u *= scale;
            q.data.straight_traverse.v *= scale;
            q.data.straight_traverse.w *= scale;
            break;
        default:
            ;
        }
    }
}

void dequeue_canons(setup_pointer settings) {

    if(debug_qc) printf("dequeueing: endpoint is now invalid\n");
    endpoint_valid = 0;

    if(qc().empty()) return;

    for(auto const &q : qc()) {
        if (q.qc_type != QCOMMENT) {
            UPDATE_TAG(q.tag);
        }

        switch(q.qc_type) {
        case QINVALID:
            if(debug_qc) printf("got invalid queued canon, skipping...\n");
            continue;
        case QARC_FEED:
            if(debug_qc) printf("issuing arc feed lineno %d\n", q.data.arc_feed.line_number);
            ARC_FEED(q.data.arc_feed.line_number, 
                     latheorigin_z(settings, q.data.arc_feed.end1), 
                     latheorigin_x(settings, q.data.arc_feed.end2), 
                     latheorigin_z(settings, q.data.arc_feed.center1),
                     latheorigin_x(settings, q.data.arc_feed.center2), 
                     q.data.arc_feed.turn, 
                     q.data.arc_feed.end3,
                     q.data.arc_feed.a, q.data.arc_feed.b, q.data.arc_feed.c, 
                     q.data.arc_feed.u, q.data.arc_feed.v, q.data.arc_feed.w,
                     q.data.arc_feed.allow_implicit_full_turn);
            break;
        case QSTRAIGHT_FEED:
            if(debug_qc) printf("issuing straight feed lineno %d\n", q.data.straight_feed.line_number);
            STRAIGHT_FEED(q.data.straight_feed.line_number, 
                          latheorigin_x(settings, q.data.straight_feed.x), 
                          q.data.straight_feed.y, 
                          latheorigin_z(settings, q.data.straight_feed.z),
                          q.data.straight_feed.a, q.data.straight_feed.b, q.data.straight_feed.c, 
                          q.data.straight_feed.u, q.data.straight_feed.v, q.data.straight_feed.w);
            break;
        case QSTRAIGHT_TRAVERSE:
            if(debug_qc) printf("issuing straight traverse lineno %d\n", q.data.straight_traverse.line_number);
            STRAIGHT_TRAVERSE(q.data.straight_traverse.line_number, 
                              latheorigin_x(settings, q.data.straight_traverse.x),
                              q.data.straight_traverse.y,
                              latheorigin_z(settings, q.data.straight_traverse.z),
                              q.data.straight_traverse.a, q.data.straight_traverse.b, q.data.straight_traverse.c, 
                              q.data.straight_traverse.u, q.data.straight_traverse.v, q.data.straight_traverse.w);
            break;
        case QSET_FEED_RATE:
            if(debug_qc) printf("issuing set feed rate\n");
            SET_FEED_RATE(q.data.set_feed_rate.feed);
            break;
        case QDWELL:
            if(debug_qc) printf("issuing dwell\n");
            DWELL(q.data.dwell.time);
            break;
        case QSET_FEED_MODE:
            if(debug_qc) printf("issuing set feed mode\n");
            SET_FEED_MODE(q.data.set_feed_mode.mode);
            break;
        case QMIST_ON:
            if(debug_qc) printf("issuing mist on\n");
            MIST_ON();
            break;
        case QMIST_OFF:
            if(debug_qc) printf("issuing mist off\n");
            MIST_OFF();
            break;
        case QFLOOD_ON:
            if(debug_qc) printf("issuing flood on\n");
            FLOOD_ON();
            break;
        case QFLOOD_OFF:
            if(debug_qc) printf("issuing flood off\n");
            FLOOD_OFF();
            break;
        case QSTART_SPINDLE_CLOCKWISE:
            if(debug_qc) printf("issuing spindle clockwise\n");
            START_SPINDLE_CLOCKWISE();
            break;
        case QSTART_SPINDLE_COUNTERCLOCKWISE:
            if(debug_qc) printf("issuing spindle counterclockwise\n");
            START_SPINDLE_COUNTERCLOCKWISE();
            break;
        case QSTOP_SPINDLE_TURNING:
            if(debug_qc) printf("issuing stop spindle\n");
            STOP_SPINDLE_TURNING();
            break;
        case QSET_SPINDLE_MODE:
            if(debug_qc) printf("issuing set spindle mode\n");
            SET_SPINDLE_MODE(q.data.set_spindle_mode.mode);
            break;
        case QSET_SPINDLE_SPEED:
            if(debug_qc) printf("issuing set spindle speed\n");
            SET_SPINDLE_SPEED(q.data.set_spindle_speed.speed);
            break;
        case QCOMMENT:
            if(debug_qc) printf("issuing comment\n");
            COMMENT(q.data.comment.comment);
            free(q.data.comment.comment);
            break;
        case QM_USER_COMMAND:
            if(debug_qc) printf("issuing mcommand\n");
            {int index=q.data.mcommand.index;
              (*(USER_DEFINED_FUNCTION[index - 100])) (index -100,
                                                    q.data.mcommand.p_number,
                                                    q.data.mcommand.q_number);
            }
            break;
        case QSTART_CHANGE:
            if(debug_qc) printf("issuing start_change\n");
            START_CHANGE();
            free(q.data.comment.comment);
            break;
        case QORIENT_SPINDLE:
            if(debug_qc) printf("issuing orient spindle\n");
            ORIENT_SPINDLE(q.data.orient_spindle.orientation, q.data.orient_spindle.mode);
            break;
        case QWAIT_ORIENT_SPINDLE_COMPLETE:
            if(debug_qc) printf("issuing wait orient spindle complete\n");
            WAIT_SPINDLE_ORIENT_COMPLETE(q.data.wait_orient_spindle_complete.timeout);
            break;
        }
    }
    qc().clear();
}

int Interp::move_endpoint_and_flush(setup_pointer settings, double x, double y) {
    double x1;
    double y1;
    double x2;
    double y2;
    double dot;

    if(qc().empty()) return 0;
    
    for(unsigned int i = 0; i<qc().size(); i++) {
        // there may be several moves in the queue, and we need to
        // change all of them.  consider moving into a concave corner,
        // then up and back down, then continuing on.  there will be
        // three moves to change.

        QueuedCanon &q = qc()[i];

        switch(q.qc_type) {
        case QARC_FEED:
        {
            double r1, r2, l1, l2;
            r1 = hypot(q.data.arc_feed.end1 - q.data.arc_feed.center1,
                       q.data.arc_feed.end2 - q.data.arc_feed.center2);
            l1 = q.data.arc_feed.original_turns;
            q.data.arc_feed.end1 = x;
            q.data.arc_feed.end2 = y;
            r2 = hypot(x - q.data.arc_feed.center1,
                       y - q.data.arc_feed.center2);
            auto circ_angle = CircleAngle(endpoint[0], endpoint[1],
                           q.data.arc_feed.center1, q.data.arc_feed.center2,
                           q.data.arc_feed.turn,
                           x, y, q.data.arc_feed.allow_implicit_full_turn);
            l2 = circ_angle.full_angle();
            if(debug_qc) printf("moving endpoint of arc lineno %d old sweep %f new sweep %f\n", q.data.arc_feed.line_number, l1, l2);

            if(fabs(r1-r2) > .01) 
                ERS(_("BUG: cutter compensation has generated an invalid arc with mismatched radii r1 %f r2 %f\n"), r1, r2);
            if(l1 != 0.0 && endpoint_valid && fabs(l2) > fabs(l1) + (settings->length_units == CANON_UNITS_MM? .0254 : .001)) {
                ERS(_("Arc move in concave corner cannot be reached by the tool without gouging"));
            }
            q.data.arc_feed.end1 = x;
            q.data.arc_feed.end2 = y;
        }
            break;
        case QSTRAIGHT_TRAVERSE:
            switch(settings->plane) {
            case CANON_PLANE_XY:
                x1 = q.data.straight_traverse.dx; // direction of original motion
                y1 = q.data.straight_traverse.dy;                
                x2 = x - endpoint[0];         // new direction after clipping
                y2 = y - endpoint[1];
                break;
            case CANON_PLANE_XZ:
                x1 = q.data.straight_traverse.dz; // direction of original motion
                y1 = q.data.straight_traverse.dx;                
                x2 = x - endpoint[0];         // new direction after clipping
                y2 = y - endpoint[1];
                break;
            default:
                ERS(_("BUG: Unsupported plane in cutter compensation"));
            }
            
            dot = x1 * x2 + y1 * y2; // not normalized; we only care about the angle
            if(debug_qc) printf("moving endpoint of traverse old dir %f new dir %f dot %f endpoint_valid %d\n", atan2(y1,x1), atan2(y2,x2), dot, endpoint_valid);

            if(endpoint_valid && dot<0) {
                // oops, the move is the wrong way.  this means the
                // path has crossed because we backed up further
                // than the line is long.  this will gouge.
                ERS(_("Straight traverse in concave corner cannot be reached by the tool without gouging"));
            }
            switch(settings->plane) {
            case CANON_PLANE_XY:
                q.data.straight_traverse.x = x;
                q.data.straight_traverse.y = y;
                break;
            case CANON_PLANE_XZ:
                q.data.straight_traverse.z = x;
                q.data.straight_traverse.x = y;
                break;
            default:
                ERS(_("BUG: Unsupported plane in cutter compensation"));
            }
            break;
        case QSTRAIGHT_FEED: 
            switch(settings->plane) {
            case CANON_PLANE_XY:
                x1 = q.data.straight_feed.dx; // direction of original motion
                y1 = q.data.straight_feed.dy;                
                x2 = x - endpoint[0];         // new direction after clipping
                y2 = y - endpoint[1];
                break;
            case CANON_PLANE_XZ:
                x1 = q.data.straight_feed.dz; // direction of original motion
                y1 = q.data.straight_feed.dx;                
                x2 = x - endpoint[0];         // new direction after clipping
                y2 = y - endpoint[1];
                break;
            default:
                ERS(_("BUG: Unsupported plane [%d] in cutter compensation"),
			settings->plane);
            }

            dot = x1 * x2 + y1 * y2;
            if(debug_qc) printf("moving endpoint of feed old dir %f new dir %f dot %f endpoint_valid %d\n", atan2(y1,x1), atan2(y2,x2), dot, endpoint_valid);

            if(endpoint_valid && dot<0) {
                // oops, the move is the wrong way.  this means the
                // path has crossed because we backed up further
                // than the line is long.  this will gouge.
                ERS(_("Straight feed in concave corner cannot be reached by the tool without gouging"));
            }
            switch(settings->plane) {
            case CANON_PLANE_XY:
                q.data.straight_feed.x = x;
                q.data.straight_feed.y = y;
                break;
            case CANON_PLANE_XZ:
                q.data.straight_feed.z = x;
                q.data.straight_feed.x = y;
                break;
            default:
                ERS(_("BUG: Unsupported plane in cutter compensation"));
            }
            break;
        default:
            // other things are not moves - we don't have to mess with them.
            ;
        }
    }
    dequeue_canons(settings);
    set_endpoint(x, y);
    return 0;
}

void set_endpoint(double x, double y) {
    if(debug_qc) printf("setting endpoint %f %f\n", x, y);
    endpoint[0] = x; endpoint[1] = y; 
    endpoint_valid = 1;
}
