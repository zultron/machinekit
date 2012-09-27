#ifndef REDISLIB_H
#define REDISLIB_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "hiredis.h"

// use the EMC_DEBUG_REDIS bit for tracing REDIS (emc/nml_intf/debugflags.h)
// or LogRedis(fmt,...); - see src/emc/rs274ngc/rs274ngc.hh


// redis primitives wrapped in redislib.c

// connect to Redis in a LinuxCNC-style fashion:
// consult inifile, else go ahead with builtin defaults
// returns NULL on failure.
extern redisContext *redis_init(const char *inifile);

// simplify sending a command a bit
extern redisReply *redis_cmd(const char *format, ...);

#ifdef __cplusplus
}
#endif




#endif
