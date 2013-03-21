
#include "config.h"
#include "rtapi.h"

#include <stdio.h>
#include <sys/stat.h>


// really in nucleus/heap.h but we rather get away with minimum include files
#ifndef XNHEAP_DEV_NAME
#define XNHEAP_DEV_NAME  "/dev/rtheap"
#endif

// if this exists, and contents is '1', it's RT_PREEMPT
#define PREEMPT_RT_SYSFS "/sys/kernel/realtime"

// dev/rtai_shm visible only after 'realtime start'
#define DEV_RTAI_SHM "/dev/rtai_shm"

int kernel_is_xenomai()
{
    struct stat sb;
    return ((stat(XNHEAP_DEV_NAME, &sb) == 0)  &&
	    ((sb.st_mode & S_IFMT) == S_IFCHR));
}

int kernel_is_rtai()
{
    struct stat sb;
    // this works only after 'realtime start'
    return ((stat(DEV_RTAI_SHM, &sb) == 0)  && ((sb.st_mode & S_IFMT) == S_IFCHR));
}

int kernel_is_rtpreempt()
{
    FILE *fd;
    int retval = 0;

    if ((fd = fopen(PREEMPT_RT_SYSFS,"r")) != NULL) {
	int flag;
	retval = ((fscanf(fd, "%d", &flag) == 1) && (flag));
	fclose(fd);
    }
    return retval;
}
