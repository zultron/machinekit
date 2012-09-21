
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <bitset>

#include "rs274ngc.hh"
#include "rs274ngc_return.hh"
#include "interp_return.hh"
#include "interp_internal.hh"
#include "rs274ngc_interp.hh"

#include "redislib.h"
#include "redis-datamodel.h"

// redis response handling is a bit lowlevel for now until
// the data model shakes out and I better understand how to 
// abstract this a bit

int Interp::get_numbered_parameter(setup_pointer settings, double *dval, int index)
{
    redisReply *reply;
    char *endptr;
    double d;
    
    if (IS_PERSISTENT_PARAM(index)) {
	if (settings->redis_context != NULL) {
	    reply = redis_cmd(REDIS_GET_NUMBERED_PARAM,index); 
	    if (reply) {
		switch (reply->type) {

		case REDIS_REPLY_STRING:
		    d =  strtod(reply->str, &endptr);
		    if (endptr == reply->str) { 
			// no conversion occured
			ERM("%s: could not convert '%s' to double",
			    __FUNCTION__, 
			    reply->str);
			freeReplyObject(reply);
			return INTERP_ERROR;
		    }
		    if (reply->str + reply->len < endptr) {
			// conversion succeeded, but not for the whole string
			// complain and continue
			Error("%s: trailing garbage after char %d in reply: '%s'",
			      __FUNCTION__, (int) (endptr-reply->str), reply->str);
		    }
		    freeReplyObject(reply);
		    *dval = d;
		    logRedis("retrieved param[%d] from redis: %g", index, d);
		    return INTERP_OK;
		    break;
		    
		case REDIS_REPLY_NIL:
		    // param not found in redis. Assume 0.
		    logRedis("param:%d not found in redis, assumming 0", index);
		    *dval = 0.0;
		    freeReplyObject(reply);
		    return INTERP_OK;

		default:
		    // other reply types are in error, so complain
		    ERS("redis: get param:%d - bad reply type %d", 
			index, reply->type);
		    break;
		}
	    }
	    ERS("redis: no reply for get param:%d %d:%s", 
		index,
		settings->redis_context->err,
		settings->redis_context->errstr);
	} else {
	    // UI param range, but not connected
	    // fall back onto internal array
	    *dval =  settings->parameters[index];
	}
    } else {
	// not in UI param range
	*dval =  settings->parameters[index];
    }
    return INTERP_OK;
}


int Interp::set_numbered_parameter(setup_pointer settings,double dval, int index, bool notify)
{
    redisReply *reply;

    settings->parameters[index] = dval;

    // if UI param and connected, mirror in redis and notify subscribers
    if ((IS_PERSISTENT_PARAM(index) || IS_REQUIRED_PARAM(index)) && 
	settings->redis_context != NULL) {
	logRedis("storing param[%d] = %g in redis", index, dval);
	reply = redis_cmd(REDIS_SET_NUMBERED_PARAM, index, dval); 
	if (reply) {
	    switch (reply->type) {
	    case REDIS_REPLY_INTEGER:
		// reply->integer meaning:
		// 1 if field is a new field in the hash and value was set.
		// 0 if field already exists in the hash and the value was updated.
		if (settings->loggingLevel > 5)
		    logRedis("hash field param %d did %s exist", index, reply->integer ? "not" :"");
	    
		// notifiy if explicitly set, or a new attribute was created
		if (notify || reply->integer) {
		    // broadcast a %d message on the param channel
		    reply = redis_cmd(REDIS_NOTIFY_PARAM_CHANGE, index);
		    if (reply && 
			reply->type == REDIS_REPLY_INTEGER) {
			if (settings->loggingLevel > 5)
			    logRedis("notified %lld subscriber(s) in params channel", reply->integer);
			freeReplyObject(reply);
			return INTERP_OK;
		    } else {
			ERS("FAIL: PUBLISH param %d (%d:%s)", index,	
			    settings->redis_context->err,
			    settings->redis_context->errstr);
		    }
		}
		freeReplyObject(reply);
		return INTERP_OK;
		break;

	    case REDIS_REPLY_STATUS: // unused
		if (strcmp(reply->str,"QUEUED") == 0) {
		    // part of MULTI/EXEC transaction bracket - continue. 
		    logRedis("-- queued ");
		    freeReplyObject(reply);
		    return INTERP_OK;
		}
		break;

	    default:
		logRedis("unhandled reply type: %d", reply->type);
		freeReplyObject(reply);

	    }
	} else {
	    logRedis("FAIL: HSET param %d %g (%d:%s)", index, dval,
		     settings->redis_context->err,
		     settings->redis_context->errstr);
	}
    }
    return INTERP_OK;
}

int Interp::redis_restore_parameters(setup_pointer settings, double parameters[])
{
    redisReply *reply;
    unsigned pair;
    char *endptr;
    unsigned long index;
    double value;
    std::bitset<RS274NGC_MAX_PARAMETERS>  missing;
    int nbad = 0;
    int nreadonly = 0; 
    int nset = 0; 
    int ndefault = 0;
    int nvolatile = 0;

    logRedis("redis_restore_parameters()");
    
    // declare all required params as MIA until found
    missing = settings->required_map;
 
    reply = redis_cmd(REDIS_GET_ALL_NUMBERED_PARAMS);
    if (reply) {
	switch (reply->type) {
	case  REDIS_REPLY_ARRAY:
	    if (reply->elements & 1) {
		ERM("expected even number of elements, got %d", reply->elements);
		freeReplyObject(reply);
		return INTERP_ERROR;
	    }
	    logRedis("retrieved %ld params", (long) reply->elements/2);

	    for (pair = 0; pair < reply->elements/2; pair++) {
		redisReply *ir,*kr;
		ir = reply->element[pair*2];
		kr = reply->element[pair*2+1];

		// FIXME robustify this.
		assert(ir->type == REDIS_REPLY_STRING);
		assert(kr->type == REDIS_REPLY_STRING);

		index = strtol(ir->str, &endptr, 10);
		value = strtol(kr->str, &endptr, 10);
		

		if (index < 0 || index >= RS274NGC_MAX_PARAMETERS) {
		    logRedis("ignoring out of range param %ld = %g", index, value);
		    nbad++;
		    continue;
		}
		if (isreadonly(index)) {
		    logRedis("ignoring readonly param %ld = %g", index, value);
		    nreadonly++;
		    continue;
		}
		missing[index] = 0;

		// also record params as persistent if we found them in the redis db
		// this will pick up params >30, < 5400 as persistent
		if (IS_VOLATILE_PARAM((int) index) && !isreadonly(index) && 
		    !settings->required_map[index]) {
		    settings->persistent_map[index] = 1;
		    nvolatile++;
		    logRedis("adding volatile param %ld = %g", index, value);
		}
		if (!isreadonly(index)) {
		    parameters[index] = value;
		    nset++;
		} else
		    nreadonly++;
	    }
	    freeReplyObject(reply);
	    if (missing.any()) { // damage reports, please
		for (int i = 0;  i < RS274NGC_MAX_PARAMETERS; i++)
		    if (missing[i]) {
			ndefault++;
			if (settings->loggingLevel > 5)
			    logRedis("missing required param %d - setting to 0.0", i);
			parameters[i] = 0.0;
		    }
	    }
	    break;
	case REDIS_REPLY_STATUS:
	    if (strcmp(reply->str, "OK")) {
		ERM("FAIL: unexpected reply to HGETALL: %d %s", reply->type, reply->str);
		freeReplyObject(reply);
		return INTERP_ERROR;
	    }
	    break;

	default:
	    logRedis("bad reply type: HGETALL param : type=%d %d %s", 
		     reply->type,settings->redis_context->err,
		     settings->redis_context->errstr);
	}
    } else {
	ERS("no reply for HGETALL param: %d %s", 
	    settings->redis_context->err,
	    settings->redis_context->errstr);
    }
    logRedis("redis_restore_parameters: %d restored, %d readonly skipped, %d bad, %d default, %d volatile ",
	     nset, nreadonly, nbad, ndefault, nvolatile);
    return INTERP_OK;
}
    
int Interp::redis_save_parameters(setup_pointer settings,const double parameters[])
{
    redisReply *reply;
    int ncmds = 0, status;
    int npersistent = 0;
    int nrequired = 0;

    // FIXME should update the persistent_map from redis
    // in case a new param was stored in redis (n > 30 && n < 5399)
    // right now, that's only picked up on next restart
    // dont think that this is actually an error

    // begin transaction, pipeline
    redisAppendCommand(settings->redis_context,"MULTI");
    for (int i = 0;  i < RS274NGC_MAX_PARAMETERS; i++) {
	if (settings->persistent_map[i] || settings->required_map[i]) {
	    if (settings->loggingLevel > 5)
		logRedis("save param %d = %g %s %s", i, parameters[i],
			 (int) settings->persistent_map[i] ? "P":"", 
			 (int) settings->required_map[i] ? "R" : "");
	    // a HMSET would be faster but a bit odd to prepare
	    redisAppendCommand(settings->redis_context, REDIS_SET_NUMBERED_PARAM, i, parameters[i] );
	    ncmds++;
	    if (settings->persistent_map[i]) npersistent++;
	    if (settings->required_map[i]) nrequired++;

	}
    }
    redisAppendCommand(settings->redis_context,"EXEC"); // comitt

    // now run through all replies including  MULTI & EXEC
    for (int i = 0 ; i < ncmds + 2; i++) {
        status = redisGetReply(settings->redis_context, (void **)&reply);
        if (status != REDIS_OK) {
	    ERM("redisGetReply cmd %d fail: status %d", i, status);
	    freeReplyObject(reply);
	    return INTERP_ERROR;
	} else {
	    switch (reply->type) {
	    case REDIS_REPLY_STATUS:
	    case REDIS_REPLY_ARRAY: // the affected keys list
		break;
	    default:
		ERM("redisGetReply type unexpected: %d", reply->type);
		freeReplyObject(reply);
		return INTERP_ERROR;
	    }
	    freeReplyObject(reply);
	}
    }
    logRedis("redis_save_parameters() saved %d params, %d persistent, %d required", 
	     ncmds, npersistent, nrequired); 
    return INTERP_OK;
}
   
