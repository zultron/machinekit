#ifndef REDIS_DATAMODEL_H
#define REDIS_DATAMODEL_H

// data model for LinuxCNC using Redis
// format strings to generate commands according to current data model

// setting/getting/notifiying about an interpreter parameter
#define REDIS_SET_NUMBERED_PARAM "HSET param %d %g" // number, value
#define REDIS_GET_NUMBERED_PARAM "HGET param %d"    // number
#define REDIS_NOTIFY_PARAM_CHANGE "PUBLISH param %d" // number
#define REDIS_GET_ALL_NUMBERED_PARAMS "HGETALL param"

// define a "tool table entry" (has nothing to do with current tool)
// NB: this is broken due to redis reply handling - FIXME
#define REDIS_SET_TOOL(toolno, x, y, z, a, b, c, u, v, w,		\
		       diameter,frontangle,backangle,orientation)	\
    REDIS_BEGIN_TRANSACTION();						\
    redis_cmd("HSET tool:%d x %f",toolno, x);				\
    redis_cmd("HSET tool:%d y %f",toolno, y);				\
    redis_cmd("HSET tool:%d z %f",toolno, z);				\
    redis_cmd("HSET tool:%d a %f",toolno, a);				\
    redis_cmd("HSET tool:%d b %f",toolno, b);				\
    redis_cmd("HSET tool:%d c %f",toolno, c);				\
    redis_cmd("HSET tool:%d u %f",toolno, u);				\
    redis_cmd("HSET tool:%d v %f",toolno, v);				\
    redis_cmd("HSET tool:%d w %f",toolno, w);				\
    redis_cmd("HSET tool:%d diameter %f",toolno, diameter);		\
    redis_cmd("HSET tool:%d frontangle %f",toolno, frontangle);		\
    redis_cmd("HSET tool:%d backangle %f",toolno, backangle);		\
    redis_cmd("HSET tool:%d orientation %d",toolno, orientation);	\
    REDIS_COMMIT_TRANSACTION() 


#endif
