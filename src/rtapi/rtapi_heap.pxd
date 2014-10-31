cdef extern from "rtapi_heap.h":
    ctypedef struct rtapi_heap:
        pass
    int rtapi_heap_init(rtapi_heap *h)
    int rtapi_heap_addmem(rtapi_heap *h, void *space, size_t size)
