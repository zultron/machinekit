# cython: binding=True 

from .rtapi_heap cimport *
from .cython_helpers cimport *  # uintptr_t, errno

import os  # strerror

class RTAPIHeapRuntimeError(MemoryError):
    """
    Thrown for problems in rtapi_heap
    """
    pass

#############################
# rtapi_heap wrapper

cdef class rtapi_heap_iter:
    cdef rtapi_heap_ptr_t _ptr
    cdef rtapi_heap_freelist_cursor _cursor
    cdef bint eol

    def __cinit__(self, object heap):
        # Cast heap intptr to rtapi_heap_ptr_t
        # (Combining the next two lines into one truncates the pointer)
        cdef uintptr_t ptr_i = heap.ptr
        self._ptr = <rtapi_heap_ptr_t> ptr_i

        # Init the cursor
        rtapi_heap_freelist_iter_init(self._ptr, &self._cursor)

        # Set flag
        self.eol = False

    def __next__(self):
        cdef size_t size
        cdef size_t offset

        if rtapi_heap_freelist_iter_next(self._ptr, &self._cursor,
                                         &size, &offset):
            raise StopIteration()

        cdef uintptr_t offset_i = <uintptr_t> offset
        return (size, offset_i)

cdef class rtapi_heap:
    cdef c_rtapi_heap *_ptr
    cdef size_t _size

    def __cinit__(self, uintptr_t ptr):
        self._size = 0
        self._ptr = <c_rtapi_heap *>ptr
        self.init()
        
    def init(self):
        cdef int res = rtapi_heap_init(self._ptr)
        # rtapi_heap_init() always returns 0

    property ptr:
        def __get__(self):
            return <uintptr_t>self._ptr

    property blocksize:
        def __get__(self):
            cdef int s = sizeof(rtapi_malloc_hdr_t)
            return s

    property headersize:
        def __get__(self):
            cdef int s = sizeof(c_rtapi_heap)
            return s

    def addmem(self, uintptr_t space, int size):
        cdef int res = rtapi_heap_addmem(self._ptr, <void *>space, <size_t>size)
        if res < 0:  # returns -errno
            raise RTAPIHeapRuntimeError(
                "rtapi_heap init failed:  %s" % os.strerror(-res), -res)
        
    def status(self):
        cdef rtapi_heap_stat hs
        cdef size_t res = rtapi_heap_status(self._ptr, &hs)
        return (<int>hs.total_avail, <int>hs.fragments, <int>hs.largest)

    def malloc(self, size_t nbytes):
        cdef void *p = rtapi_malloc(self._ptr, nbytes)
        if p == NULL:
            raise RTAPIHeapRuntimeError(
                "rtapi_malloc failed: out of memory")

        return <uintptr_t> p

    def free(self, uintptr_t ap):
        rtapi_free(self._ptr, <void *>ap)

    def calloc(self, size_t nelem, size_t elsize):
        cdef void *p = rtapi_calloc(self._ptr, nelem, elsize)
        if p == NULL:
            raise RTAPIHeapRuntimeError(
                "rtapi_calloc failed: out of memory")

        return <uintptr_t> p
        
    def realloc(self, uintptr_t ptr, size_t size):
        cdef void *p = rtapi_realloc(self._ptr, <void *>ptr, size)
        if p == NULL:
            raise RTAPIHeapRuntimeError(
                "rtapi_realloc failed: out of memory")

        return <uintptr_t> p
        
    def getptr(self, size_t offset):
        return <uintptr_t>heap_ptr(self._ptr, offset)

    def offset(self, uintptr_t p):
        return <int>heap_off(self._ptr, <void *>p)

    def __iter__(self):
        return rtapi_heap_iter(self)

