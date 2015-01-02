import logging
from machinekit.rtapi.shm import MKSHMSegment
from machinekit.rtapi.rtapi_heap_bindings import rtapi_heap
PARAMETER_HEAP_SIZE = 1024*100  # Allocate 100KiB by default
PARAMETER_HEAP_KEY = 0x00cebad0  # FIXME this goes in rtapi/config/parameter.h

class SHMParameterError(RuntimeError):
    '''
    Raised for exceptions related to parameter storage in shm
    '''
    pass

class SHMParameterHeap(MKSHMSegment):
    requested_size = PARAMETER_HEAP_SIZE
    magic = PARAMETER_HEAP_KEY
    name = "parameter"
    log = logging.getLogger(__name__)

    def __init__(*args, **kwargs):
        '''
        Initialize shm segment for parameter storage

        Uses same parameters as `MKSHMSegment`, and:

        - `header_in_shm`:  If `True`, put the heap header struct in shm

        - `header_ptr`: If `header_in_shm is `False` (the default),
          this should be a `uintptr_t` to the header struct.

        The `heap` attribute is the `rtapi_heap` object.
        '''
        # If header_in_shm is True, put the rtapi_heap struct at the
        # beginning of the shm seg
        self.header_in_shm = kwargs.pop('header_in_shm', False)
        # Otherwise, we need a pointer to rtapi_heap struct
        self._header_ptr = kwargs.pop('header_ptr', None)
        if self.header_in_shm is False:
            if self._header_ptr is None:
                raise SHMParameterError(
                    "Need pointer to heap header when header_in_shm is False")
        else:
            if self._header_ptr is not None:
                raise SHMParameterError(
                    "Heap header pointer defined when header_in_shm is False")
        self.heap = None

        super(SHMParameterHeap, self).__init__(*args, **kwargs)

    @property
    def header_ptr(self):
        if self._header_ptr is None:
            assert self.header_in_shm
            self._header_ptr = self.ptr
        return self._header_ptr
        
    @property
    def arena_size(self):
        if self.header_in_shm:
            return self.size - self.heap.headersize
        else:
            return self.size

    @property
    def arena_ptr(self):
        if self.header_in_shm:
            return self.ptr + self.heap.headersize
        else:
            return self.ptr

    def new(self):
        super(SHMParameterHeap, self).new()

        # Init shm segment
        self.heap = rtapi_heap(self.header_ptr)

        # Add heap memory
        self.heap.addmem(self.arena_ptr, self.arena_size)

        return self

class SHMParameter(object):

    def __init__(self, heap, sections, name, value=None):
        self.heap = heap
        self.sections = sections
        self.name = name
        self.value = value




# /tmp/machinekit/machinekit/src/rtapi/shmdrv/shmdrv_api.pyx
# /tmp/machinekit/machinekit/src/rtapi/rtapi_heap_bindings.pyx
# /tmp/machinekit/machinekit/lib/python/machinekit/rtapi/shm.py
