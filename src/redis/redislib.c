// hiredis/LinuxCNC glue

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "inifile.h"
#include "rtapi.h"		// only rtapi_print_msg 
#include "hiredis.h"
#include "redislib.h"

#define MIN_TIMEOUT 0.5  // seconds to connect to redis server

static int emc_debug;

static int redis_port = 6379;
static const char *redis_host = "127.0.0.1";
static const char *redis_password;
static double redis_timeout = 1.5;

static redisContext *redis_context;


redisContext *redis_init(const char *inifile)
{
    FILE *fp;
    const char *s;
    struct timeval timeout = { 2, 0 };
    redisReply *reply;

    if (redis_context) {
	rtapi_print_msg(RTAPI_MSG_DBG, "%s: already connected to server at %s:%d\n",
			__FUNCTION__, redis_host, redis_port);
	return redis_context;
    }
    if (!inifile) 
	inifile = getenv("INI_FILE_NAME");
    if (!inifile) {
	rtapi_print_msg(RTAPI_MSG_DBG, "%s: no inifile, assuming redis server at %s:%d\n",
			__FUNCTION__, redis_host, redis_port);
    } else {
	// see if overridden in ini
	if ((fp = fopen(inifile, "r")) != NULL) {
	    iniFindInt(fp, "DEBUG", "EMC", &emc_debug);
	    iniFindInt(fp, "PORT", "REDIS", &redis_port);
	    if (iniFindDouble(fp, "TIMEOUT", "REDIS", &redis_timeout) && redis_timeout > MIN_TIMEOUT) {
		timeout.tv_sec = floor(redis_timeout);
		timeout.tv_usec = (redis_timeout-timeout.tv_sec) * 1000000;
	    }
	    if ((s = iniFind(fp, "HOST", "REDIS"))) {
		redis_host = strdup(s);
	    }
	    if ((s = iniFind(fp, "PASSWORD", "REDIS"))) {
		redis_password = strdup(s);
	    }
	    fclose(fp);
	} else {
	    rtapi_print_msg(RTAPI_MSG_WARN, "%s: cant open inifile '%s', assuming server at %s:%d\n",
			    __FUNCTION__, inifile, redis_host, redis_port);
	}
    }
    
    // connect to redis server
    redis_context = redisConnectWithTimeout(redis_host, redis_port, timeout);
    if (redis_context->err) {
	rtapi_print_msg(RTAPI_MSG_ERR, "%s: cant connect to Redis server %s:%d - %s\n",
			__FUNCTION__, redis_host, redis_port, redis_context->errstr);
	return NULL;
    }
    
    // authenticate if [REDIS]PASSWORD given
    if (redis_password) {
	reply = redisCommand(redis_context,"AUTH %s", redis_password);
	if (reply != NULL) {
	    freeReplyObject(reply);
	} else {
	    rtapi_print_msg(RTAPI_MSG_ERR, "%s: no response to AUTH %s request: %s\n",
			__FUNCTION__, redis_password, redis_context->errstr);
	    return NULL;
	}
    }

    // make sure she talks to us
    reply = redisCommand(redis_context, "PING");
    freeReplyObject(reply);
    if (redis_context->err) {
	rtapi_print_msg(RTAPI_MSG_ERR, "%s: Redis server doesnt answer to 'PING': %s\n",
			__FUNCTION__, redis_context->errstr);
	return NULL;
    }
    return redis_context;
}


// NB: explicit freeReplyObject(reply) required
redisReply *redis_cmd(const char *format, ...) 
{
    va_list ap;
    redisReply *reply;
    va_start(ap,format);
    reply =  (redisReply *) redisvCommand(redis_context, format,  ap);
    va_end(ap);
    if (!reply) { 
	rtapi_print_msg(RTAPI_MSG_ERR, "%s: no reply for command:\n",
			__FUNCTION__);
	va_start(ap,format);
	rtapi_print_msg(RTAPI_MSG_ERR, format, ap);
	va_end(ap);
	return NULL;
    }
    return reply;
}
