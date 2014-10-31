
from .rtapi_int cimport *
from .cython_helpers cimport *

cdef extern from "ring.h":
    int SIZE_ALIGN(x)

    ctypedef struct ringheader_t:
        int use_wmutex
        int32 refcount
        int32 reader
    ctypedef struct ringbuffer_t:
        ringheader_t   *header
    ctypedef struct ringtrailer_t:
        pass

    void ringheader_init(ringheader_t *ringheader, int flags,
                         size_t size, size_t sp_size)
    void ringbuffer_zero(ringheader_t *ringheader)
    void ringbuffer_init(ringheader_t *ringheader, ringbuffer_t *ring)
    
