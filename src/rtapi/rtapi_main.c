/********************************************************************
* Description:  rtapi_main.c
*
*               This file, 'rtapi_main.c', implements the RTAPI
*               rtapi_app_main() and rtapi_app_exit() functions
*               for userspace thread systems.
*
*               It should not be used for kernel thread systems.
*
********************************************************************/

#include "config.h"

#include <sys/ipc.h>		/* IPC_* */
#include <sys/shm.h>		/* shmget() */
#include <stdlib.h>		/* rand_r() */
#include <unistd.h>		/* getuid(), getgid(), sysconf(),
				   ssize_t, _SC_PAGESIZE */

#include "rtapi.h"		/* RTAPI realtime OS API */
#include "rtapi_app.h"		/* RTAPI realtime module decls */
#include "rtapi_common.h"       /* rulapi_data_t */
#include "rtapi_kdetect.h"      /* environment autodetection */


MODULE_AUTHOR("Michael Haberler");
MODULE_DESCRIPTION("RTAPI stubs for userland threadstyles");
MODULE_LICENSE("GPL2 or later");

static int check_compatible();
static int rulapi_shm_init(key_t key, rulapi_data_t **rulapi_data);
static int rulapi_shm_free(int shm_id, rulapi_data_t *rulapi_data);
static int rulapi_shmid;

int rtapi_app_main(void)
{
    rtapi_print_msg(RTAPI_MSG_INFO,"RTAPI %s %s startup\n", 
		    rtapi_switch->thread_flavor_name, GIT_VERSION);

    if ((rulapi_shmid = rulapi_shm_init(RULAPI_KEY, &rulapi_data)) < 0) {
	return rulapi_shmid;
    }
    // the globally shared segment */
    init_rulapi_data(rulapi_data);

    // investigate what we're dealing with and fail
    // rtapi_app_main if the build of this object and the environemt
    // is incompatible
    return check_compatible();
}

void rtapi_app_exit(void)
{
    rtapi_print_msg(RTAPI_MSG_INFO,"RTAPI %s %s exit\n",
		    rtapi_switch->thread_flavor_name, GIT_VERSION);
    rulapi_shm_free(rulapi_shmid, rulapi_data);
    rulapi_data = NULL;
}

#if !defined(THREAD_FLAVOR_ID)
#error "THREAD_FLAVOR_ID is not defined!"
#endif

#if THREAD_FLAVOR_ID == RTAPI_XENOMAI_USER_ID
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

#elif THREAD_FLAVOR_ID ==  RTAPI_RT_PREEMPT_USER_ID

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

#elif THREAD_FLAVOR_ID == RTAPI_POSIX_ID

static int check_compatible()
{
    return 0; // no prerequisites
}

#else

#error "THREAD_FLAVOR_ID not set"
#endif


static int rulapi_shm_init(key_t key, rulapi_data_t **rulapi_data) 
{
    int retval, shm_id;
    int size = sizeof(rulapi_data_t);
    struct shmid_ds d;
    void *rd;

    if ((shm_id = shmget(key, size, RULAPI_DATA_PERMISSIONS)) > -1) {
	rtapi_print_msg(RTAPI_MSG_ERR, "%s: RTAPI data segment already exists\n", 
			__FUNCTION__);
	return -EEXIST;
    }
    if (errno != ENOENT) {
	rtapi_print_msg(RTAPI_MSG_ERR, "%s:shmget(): unexpected - %d - %s\n", 
			errno,strerror(errno));
	return -EINVAL;
    }
    // nope, doesnt exist - create
    if ((shm_id = shmget(key, size, RULAPI_DATA_PERMISSIONS | IPC_CREAT)) == -1) {
	rtapi_print_msg(RTAPI_MSG_ERR, "%s: shmget(key=0x%x, IPC_CREAT): %d - %s\n", 
			__FUNCTION__, key, errno, strerror(errno));
	return -EINVAL;
    }
    // get actual user/group and drop to ruid/rgid so removing is
    // always possible
    if ((retval = shmctl(shm_id, IPC_STAT, &d)) < 0) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s:  shm_ctl(key=0x%x, IPC_STAT) failed: %d - %s\n", 
			__FUNCTION__, key, errno, strerror(errno));  
	return -EINVAL;
    } else {
	// drop permissions of shmseg to real userid/group id
	if (!d.shm_perm.uid) { // uh, root perms 
	    d.shm_perm.uid = getuid();
	    d.shm_perm.gid = getgid();
	    if ((retval = shmctl(shm_id, IPC_SET, &d)) < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
				"%s: shm_ctl(key=0x%x, IPC_SET) "
				"failed: %d '%s'\n", 
				__FUNCTION__, key, errno, 
				strerror(errno));
		return -EINVAL;
	    } 
	}
    }
  
    // and map it into process space 
    rd = shmat(shm_id, 0, 0);
    if (((ssize_t) rd) == -1) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: shmat(%d) failed: %d - %s\n",
			__FUNCTION__, shm_id, 
			errno, strerror(errno));
	return -EINVAL;
    }
    // Touch each page by zeroing the whole mem
    memset(rd, 0, size);
    *rulapi_data = rd;
    return shm_id;
}

static int rulapi_shm_free(int shm_id, rulapi_data_t *rulapi_data) 
{
    struct shmid_ds d;
    int r1, r2;

    /* unmap the shared memory */
    r1 = shmdt(rulapi_data);
    if (r1 < 0) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: shmdt(%p) failed: %d - %s\n",
			__FUNCTION__, rulapi_data, errno, strerror(errno));      
	return -EINVAL;
    }
    /* destroy the shared memory */
    r2 = shmctl(shm_id, IPC_STAT, &d);
    if (r2 < 0) {
	rtapi_print_msg(RTAPI_MSG_ERR,
			"%s: shm_ctl(%d, IPC_STAT) failed: %d - %s\n", 
			__FUNCTION__, shm_id, errno, strerror(errno));      
    }
    if(r2 == 0 && d.shm_nattch == 0) {
	r2 = shmctl(shm_id, IPC_RMID, &d);
	if (r2 < 0) {
	    rtapi_print_msg(RTAPI_MSG_ERR,
			    "%s: shm_ctl(%d, IPC_RMID) failed: %d - %s\n", 
			    __FUNCTION__, shm_id, errno, strerror(errno));   
	}
    }  
    return 0;
}
