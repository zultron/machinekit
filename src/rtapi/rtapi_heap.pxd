cdef extern from "rtapi_heap_private.h":
    struct rtapi_malloc_header_s:
        size_t next
        unsigned size
    union rtapi_malloc_header:
        rtapi_malloc_header_s s
    ctypedef rtapi_malloc_header rtapi_malloc_hdr_t
    cdef struct c_rtapi_heap "rtapi_heap":
        rtapi_malloc_hdr_t base
        size_t free_p
        size_t arena_size
    ctypedef c_rtapi_heap *rtapi_heap_ptr_t
    void *heap_ptr(c_rtapi_heap *base, size_t offset)
    size_t heap_off(c_rtapi_heap *base, void *p)

# cdef extern from "rtapi_heap.h":
    ctypedef struct rtapi_heap_stat:
        size_t total_avail
        size_t fragments
        size_t largest

    void *rtapi_malloc(c_rtapi_heap *h, size_t nbytes)
    void *rtapi_calloc(c_rtapi_heap *h, size_t n, size_t size)
    void *rtapi_realloc(c_rtapi_heap *h, void *p, size_t size)
    void  rtapi_free(c_rtapi_heap *h, void *p)
    size_t rtapi_allocsize(void *p)

    int rtapi_heap_init(c_rtapi_heap *h)
    int rtapi_heap_addmem(c_rtapi_heap *h, void *space, size_t size)
    size_t rtapi_heap_status(c_rtapi_heap *h, rtapi_heap_stat *hs)

    # Python iterator helpers
    struct rtapi_heap_freelist_cursor:
        pass
    ctypedef rtapi_heap_freelist_cursor rtapi_heap_freelist_cursor
    void rtapi_heap_freelist_iter_init(c_rtapi_heap *h,
                                       rtapi_heap_freelist_cursor *cursor)
    int rtapi_heap_freelist_iter_next(c_rtapi_heap *h,
                                      rtapi_heap_freelist_cursor *cursor,
                                      size_t *size, size_t *offset)
