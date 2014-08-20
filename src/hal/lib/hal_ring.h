#ifndef HAL_RING_H
#define HAL_RING_H

#include <rtapi.h>
#include <ring.h>
#include <multiframe.h>


RTAPI_BEGIN_DECLS

// a ring buffer exists always relative to a local instance
// it is 'owned' by hal_lib since because rtapi_shm_new() requires a module id
// any module may attach to it.

typedef struct {
    char name[HAL_NAME_LEN + 1]; // ring HAL name
    int next_ptr;		 // next ring in used/free lists
    int ring_id;                 // as per alloc bitmap
    int ring_shmkey;             // RTAPI shm key - if in shmseg
    int total_size;              // size of shm segment allocated
    unsigned ring_offset;        // if created in HAL shared memory
    unsigned flags;
    int handle;                  // unique ID
} hal_ring_t;

// some components use a fifo and a scratchpad shared memory area,
// like sampler.c and streamer.c. ringbuffer_t supports this through
// the optional scratchpad, which is created if spsize is > 0
// on hal_ring_new(). The scratchpad size is recorded in
// ringheader_t.scratchpad_size.

// generic ring methods for all modes:

// create a named ringbuffer, owned by hal_lib
//
// mode is an or of:

// exposed in ringheader_t.type:
// #define RINGTYPE_RECORD    0
// #define RINGTYPE_MULTIPART RTAPI_BIT(0)
// #define RINGTYPE_STREAM    RTAPI_BIT(1)

// mode flags passed in by ring_new
// exposed in ringheader_t.{use_rmutex, use_wmutex, alloc_halmem}
// #define USE_RMUTEX       RTAPI_BIT(2)
// #define USE_WMUTEX       RTAPI_BIT(3)
// #define ALLOC_HALMEM     RTAPI_BIT(4)

// spsize > 0 will allocate a shm scratchpad buffer
// accessible through ringbuffer_t.scratchpad/ringheader_t.scratchpad


// named HAL rings are owned by the HAL_LIB_<pid> RTAPI module
// components do not make sense as owners since their lifetime
// might be shorter than the ring

int hal_ring_new(const char *name, int size, int spsize, int mode);

// delete a ring buffer.
// will fail if the refcount is > 0 (meaning the ring is still attached somewhere).
int hal_ring_delete(const char *name);

// make an existing ringbuffer accessible to a component
// rb must point to storage of type ringbuffer_t.
// Increases the reference count.
// store halring flags in *flags if non-zero.
int hal_ring_attach(const char *name, ringbuffer_t *rb, unsigned *flags);

// detach a ringbuffer. Decreases the reference count.
int hal_ring_detach(const char *name, ringbuffer_t *rb);

// not part of public API. Use with HAL lock engaged.
hal_ring_t *halpr_find_ring_by_name(const char *name);

RTAPI_END_DECLS

#endif /* HAL_RING_PRIV_H */
