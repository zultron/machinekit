/********************************************************************
* Description:  rtapi_main.c
*
*               This file, 'rtapi_main.c', implements the
*               rtapi_app_main() and rtapi_app_exit() functions
*               for userspace thread systems.
*
*               It should not be used for kernel thread systems.
*
********************************************************************/

#include "config.h"
#include "rtapi.h"		/* RTAPI realtime OS API */
#include "rtapi_app.h"		/* RTAPI realtime module decls */
#include "rtapi_kdetect.h"      /* environment autodetection */

MODULE_AUTHOR("Michael Haberler");
MODULE_DESCRIPTION("RTAPI stubs for userland threadstyles");
MODULE_LICENSE("GPL2 or later");

static int check_compatible();

int rtapi_app_main(void)
{
    unsigned long features;
    int retval = 0;

    rtapi_print_msg(RTAPI_MSG_INFO,"RTAPI %s startup\n", GIT_VERSION);

    // investigate what we're dealing with and fail
    // rtapi_app_main if the build of this object and the environemt
    // is incompatible
    return check_compatible();
}

void rtapi_app_exit(void)
{
    rtapi_print_msg(RTAPI_MSG_INFO,"RTAPI exit\n");
}



// fudge it for testing - please fix by proper include/define
#define XENOMAI_USER 1
#define RT_PREEMPT_USER 2

#define THREADSTYLE XENOMAI_USER
//#define THREADSTYLE  RT_PREEMPT_USER


#if THREADSTYLE == XENOMAI_USER
static int check_compatible()
{
    int retval = 0;

    if (!kernel_is_xenomai()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started Xenomai RTAPI on a non-Xenomai kernel\n",
			__FUNCTION__);
	retval--;
    }

    if (kernel_is_rtai()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started Xenomai RTAPI on an RTAI kernel\n",
			__FUNCTION__);
	retval--;
    }
    if (kernel_is_rtpreempt()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started Xenomai RTAPI on an RT PREEMPT kernel\n",
			__FUNCTION__);
	retval--;
    }
    return retval;
}

#elif THREADSTYLE == RT_PREEMPT_USER

static int check_compatible()
{
   int retval = 0;

    if (!kernel_is_rtpreempt()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started RT_PREEMPT RTAPI on a non-RT PREEMPT kernel\n",
			__FUNCTION__);
	retval--;
    }

    if (kernel_is_rtai()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started RT_PREEMPT RTAPI on an RTAI kernel\n",
			__FUNCTION__);
	retval--;
    }
    if (kernel_is_xenomai()) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: started RT_PREEMPT RTAPI on a Xenomai kernel\n",
			__FUNCTION__);
	retval--;
    }
    return retval;
}

#else

#error "THREADSTYLE not set"
#endif
