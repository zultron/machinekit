/********************************************************************
* Description:  rtapi_support.c
*               This file, 'rtapi_support.c', implements the messaging
*               functions for both kernel and userland thread
*               systems.  See rtapi.h for more info.
*
*               Other than the rest of RTAPI, these functions are linked
*               into the instance module which is loaded before rtapi.so/ko
*               so they are available and message level set before
*               RTAPI starts up
*
*     Copyright 2006-2013 Various Authors
* 
*     This program is free software; you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation; either version 2 of the License, or
*     (at your option) any later version.
* 
*     This program is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
* 
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
********************************************************************/


#include "config.h"
#include "rtapi.h"
#include "rtapi/shmdrv/shmdrv.h"
#include "ring.h"

#define RTPRINTBUFFERLEN 1024

#ifdef MODULE
#include "rtapi_app.h"

#include <stdarg.h>		/* va_* */
#include <linux/kernel.h>	/* kernel's vsnprintf */

#define MSG_ORIGIN MSG_KERNEL

#else  /* user land */

#include <stdio.h>		/* libc's vsnprintf() */
#include <sys/types.h>
#include <unistd.h>

#ifdef RTAPI
#define MSG_ORIGIN MSG_RTUSER
#else
#define MSG_ORIGIN MSG_ULAPI
#endif

#endif

// these message levels are used in RTAPI and ULAPI
// respectively until the global segment is attached;
// thereafter switch to using the message levels from there.
#ifdef RTAPI
static int rt_msg_level = RTAPI_MSG_INFO;    // RTAPI (u+k)
#else
static int ulapi_msg_level = RTAPI_MSG_INFO; // ULAPI
#endif

#ifdef ULAPI
ringbuffer_t rtapi_message_buffer;   // rtapi_message ring access strcuture
# else
extern ringbuffer_t rtapi_message_buffer;
#endif

static char logtag[TAGSIZE];

// switch to exclusively using the ringbuffer from RT
#define USE_MESSAGE_RING 1

void vs_ring_write(msg_level_t level, const char *format, va_list ap)
{
    int n;
    rtapi_msgheader_t *msg;
#if defined(RTAPI) && defined(BUILD_SYS_USER_DSO)
    static pid_t rtapi_pid;
    if (rtapi_pid == 0)
	rtapi_pid = getpid();

#endif

    if (global_data) {
	// one-time initialisation
	if (!rtapi_message_buffer.header) {
	    ringbuffer_init(&global_data->rtapi_messages, &rtapi_message_buffer);
	}
	if (rtapi_mutex_try(&rtapi_message_buffer.header->wmutex)) {
	    global_data->error_ring_locked++;
	    return;
	}
	// zero-copy write
	// reserve space in ring:
	if (record_write_begin(&rtapi_message_buffer,
				     (void **) &msg, 
				     sizeof(rtapi_msgheader_t) + RTPRINTBUFFERLEN)) {
	    global_data->error_ring_full++;
	    rtapi_mutex_give(&rtapi_message_buffer.header->wmutex);
	    return;
	}
	msg->origin = MSG_ORIGIN;
#if defined(RTAPI) && defined(BUILD_SYS_KBUILD)
	msg->pid = 0;
#endif
#if defined(RTAPI) && defined(BUILD_SYS_USER_DSO)
	msg->pid =  rtapi_pid;
#endif
#if defined(ULAPI)
	msg->pid  = getpid();
#endif
	msg->level = level;
	msg->encoding = MSG_ASCII;
	strncpy(msg->tag, logtag, sizeof(msg->tag));

	n = vsnprintf(msg->buf, RTPRINTBUFFERLEN, format, ap);
	// commit write
	record_write_end(&rtapi_message_buffer, (void *) msg,
			       sizeof(rtapi_msgheader_t) + n + 1); // trailing zero
	rtapi_mutex_give(&rtapi_message_buffer.header->wmutex);
    }
}

#ifdef MODULE
void default_rtapi_msg_handler(msg_level_t level, const char *fmt,
			      va_list ap) {
    char buf[RTPRINTBUFFERLEN];
    vsnprintf(buf, RTPRINTBUFFERLEN, fmt, ap);
    vs_ring_write(level, buf, ap);
}

#else /* user land */
void default_rtapi_msg_handler(msg_level_t level, const char *fmt,
			       va_list ap) {
    // during startup the global segment might not be
    // available yet, so use stderr until then
    if (MMAP_OK(global_data)) {
	vs_ring_write(level, fmt, ap);
    } else {
	vfprintf(stderr, fmt, ap);
    }
}
#endif

static rtapi_msg_handler_t rtapi_msg_handler = default_rtapi_msg_handler;

rtapi_msg_handler_t rtapi_get_msg_handler(void) {
    return rtapi_msg_handler;
}

void rtapi_set_msg_handler(rtapi_msg_handler_t handler) {
    if (handler == NULL)
	rtapi_msg_handler = default_rtapi_msg_handler;
    else
	rtapi_msg_handler = handler;
}


// rtapi_get_msg_level and rtapi_set_msg_level moved here
// since they access the global segment 
// which might not exist during first use
// assure we can use message levels before global_data is set up

static int get_msg_level(void)
{
#if RTAPI
    if (global_data == 0)
	return rt_msg_level;
    else
	return global_data->rt_msg_level;
#else
    return ulapi_msg_level;
#endif
}

static int set_msg_level(int new_level)
{
    int old_level;

#if RTAPI
    if (global_data) {
	old_level = global_data->rt_msg_level;
	global_data->rt_msg_level = new_level;
    } else {
	old_level = rt_msg_level;
	rt_msg_level = new_level;
    }
    return old_level;
#else
    old_level = ulapi_msg_level;
    ulapi_msg_level = new_level;
    return old_level;
#endif
}

int rtapi_set_msg_level(int level) {
    int oldlevel;
    if ((level < RTAPI_MSG_NONE) || (level > RTAPI_MSG_ALL)) {
	return -EINVAL;
    }
    oldlevel = set_msg_level(level);
    return oldlevel;
}

int rtapi_get_msg_level() {
    return get_msg_level();
}

void rtapi_print(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    rtapi_msg_handler(RTAPI_MSG_ALL, fmt, args);
    va_end(args);
}

void rtapi_print_msg(int level, const char *fmt, ...) {
    va_list args;

    if ((level <= rtapi_get_msg_level()) && 
	(rtapi_get_msg_level() != RTAPI_MSG_NONE)) {
	va_start(args, fmt);
	rtapi_msg_handler(level, fmt, args);
	va_end(args);
    }
}

int rtapi_snprintf(char *buf, unsigned long int size,
		   const char *fmt, ...) {
    va_list args;
    int result;

    va_start(args, fmt);
    result = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return result;
}

int rtapi_vsnprintf(char *buf, unsigned long int size, const char *fmt,
		    va_list ap) {
    return vsnprintf(buf, size, fmt, ap);
}

int rtapi_set_logtag(const char *fmt, ...) {
    va_list args;
    int result;

    va_start(args, fmt);
    result = vsnprintf(logtag, sizeof(logtag), fmt, args);
    va_end(args);
    return result;
}

const char *rtapi_get_logtag(void) {
    return logtag;
}


#ifdef RTAPI
EXPORT_SYMBOL(rtapi_get_msg_handler);
EXPORT_SYMBOL(rtapi_set_msg_handler);
EXPORT_SYMBOL(rtapi_print_msg);
EXPORT_SYMBOL(rtapi_print);
EXPORT_SYMBOL(rtapi_snprintf);
EXPORT_SYMBOL(rtapi_vsnprintf);
EXPORT_SYMBOL(rtapi_set_msg_level);
EXPORT_SYMBOL(rtapi_get_msg_level);
EXPORT_SYMBOL(rtapi_set_logtag);
EXPORT_SYMBOL(rtapi_get_logtag);
#endif
