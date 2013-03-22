/********************************************************************
* Description:  ulapi_main.c
*
*               This file, 'ulapi_main.c', implements the ULAPI
*               rtapi_app_main() and rtapi_app_exit() functions
*               for hal_lib.c support.
*
*               It is used on any thread system and in usermode 
*               processes only.
*
*               the primary purpose of ulapi_main.c is to set the
*               rtapi_instance variable.
*
*               This does not follow the module conventions since
*               it is loaded from usermode hal_lib.c only, not in
*               an RT context.
*
********************************************************************/
#include "config.h"
#include <stdio.h>		/* fprintf() */
#include <stdlib.h>		/* getenv() */
#include "rtapi.h"		/* rtapi_print_msg */

// intentionally extern so the rest of ulapi.so can 'see' it
int rtapi_instance;

// we cannot yet use rtapi_print_msg here; for some reason rtapi_switch is
// still 0 here - FIXME

static int verbose;

int up_api_main(void)
{
    const char *instance = getenv("INSTANCE");
    verbose = (getenv("DEBUG") != NULL);

    if (instance != NULL)
    	rtapi_instance = atoi(instance);

    if (verbose)
	fprintf(stderr, "UP API %s instance:%d startup\n", 
		GIT_VERSION, rtapi_instance);
    return 0;
}

void up_api_exit(void)
{
    if (verbose)
	fprintf(stderr, "UP API %s %s instance:%d exit\n",
		rtapi_switch->thread_flavor_name, 
		GIT_VERSION, rtapi_instance);
}
