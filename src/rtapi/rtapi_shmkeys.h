#ifndef _RTAPI_SHMKEYS_H
#define _RTAPI_SHMKEYS_H

// the single place for shared memory keys

// convention: allocate a new key such that its
// least significant byte is zero, which is used for 
// instance management.

// formerly emcmotcfg.h
/*
  Shared memory keys for simulated motion process. No base address
  values need to be computed, since operating system does this for us
  */
#define DEFAULT_MOTION_SHMEM_KEY 100


#endif _RTAPI_SHMKEYS_H
