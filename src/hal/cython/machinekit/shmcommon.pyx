# cython: binding=True 

from .shmcommon cimport *
from .global_data cimport *

import logging, os
import types

def init():
    r = c_shm_common_init() # always returns 0

class SHMCommonRuntimeError(RuntimeError):
    """
    Thrown for problems in shm_common
    """
    pass

#############################
# shmdrv wrappers

def shmdrv_available():
    return c_shmdrv_available()

def shmdrv_gc():
    cdef int res
    res = c_shmdrv_gc()
    if res == -1:  # -EPERM
        raise SHMCommonRuntimeError(
            "Accessing shmdrv device node: permission denied")
    elif res != 0:
        raise SHMCommonRuntimeError(
            "Accessing shmdrv device node: %s" % os.strerror(res))


def shmdrv_loaded():
    return c_shmdrv_loaded != 0

#############################
# POSIX wrappers

def set_name_format(str prefix):
    """set the POSIX shm name format"""
    if len(prefix) > SHM_NAME_PREFIX_MAXLEN:
        raise SHMCommonRuntimeError(
            "shm prefix length exceeds maximum:  %d > %d" %
            (len(prefix), SHM_NAME_PREFIX_MAXLEN))
    c_shm_common_set_name_format(prefix)

cpdef segment_posix_name(int key):
    # FIXME The following line raises compile error indicating
    # SHM_NAME_MAXLEN:
    #    Not allowed in a constant expression
    # cdef char segment_name[SHM_NAME_MAXLEN]
    # Is this related?  http://cython-devel.gmang.org/2013-May/003635.html
    # So this is the ugliness for now:
    cdef char segment_name[136]
    c_shm_common_segment_posix_name(segment_name, key)
    cpdef bytes res = segment_name
    return res

def exists(int key):
    cdef object result = <bint> c_shm_common_exists(key)
    return result

#############################
# shmdrvapi C Wrapper
#
# The _shm_seg_wrapper class wraps the basic parameters into an object
# that can be passed around in python, even the non-python void*
# attribute.
#
# While we're at it, make this an extra-thin wrapper around the main
# shmdrv API functions.
cdef class _shm_seg_wrapper:
    cdef int _key
    cdef int _size
    cdef void *_ptr
    def __repr__(self):
        return "<shm segment %s>" % self._key
    def __cinit__(self):
        self._key = 0
        self._size = 0
    property size:
        def __get__(self): return self._size
    property key:
        def __get__(self): return self._key
    

cdef _shm_seg_wrapper _new(int key, int size, int create):
    cdef _shm_seg_wrapper seg
    cdef int res
    # set up wrapper
    seg = _shm_seg_wrapper()
    seg._key = key
    seg._size = size
    # get new shm seg
    res = c_shm_common_new(seg._key, &seg._size, &seg._ptr, create)
    if res < 0:
        raise SHMCommonRuntimeError(
            "Failed to %s shm object, key=%08x:  %s" %
            (("attach","create")[create], key, os.strerror(-res)))

    if create == 1 and  res == 0:
        raise SHMCommonRuntimeError(
            "Failed to create new shm object:  key=%08x already exists" % key)

    return seg

cdef int _detach(_shm_seg_wrapper seg) except -1:
    cdef int res
    res = c_shm_common_detach(seg._size, seg._ptr)
    if res == -1:
        PyErr_SetFromErrno(PyExc_IOError)
    return res

cdef int _unlink(_shm_seg_wrapper seg) except -1:
    res = c_shm_common_unlink(seg._key)
    if res == -1:
        PyErr_SetFromErrno(PyExc_IOError)
    return res

#######################################
# Wrapper wrapper
#
# This python class wraps the C class, providing a more pythonic
# interface.
class shm_seg(object):

    def __init__(self):
        self.log = logging.getLogger("machinekit.rtapi.shm.shm_seg")
        self.seg = None

    @property
    def key(self):
        if self.seg is None:  return None
        return self.seg.key

    @property
    def size(self):
        if self.seg is None:  return None
        return self.seg.size

    @property
    def name(self):
        if self.seg is None:  return None
        return segment_posix_name(self.seg.key)

    def new(self, int key, int size, bint create = 1):
        """Create new shm segment with key and size; """

        # if self._ptr != NULL:
        # if self._ptr is not None:
        if self.seg is not None:
            raise SHMCommonRuntimeError(
                "Create or attach failed:  object already initialized: "
                "object key=%08x, size=%d; request key=%08x, size=%d" %
                (self.key, self.size, key, size))

        self._key = key
        self._size = size

        self.seg = _new(self._key, self._size, create)

        self.log.debug("%s new shm seg, key=%08x, size %d" %
                       (('Attached','Created')[create],
                        self.seg.key, self.seg.size))

        return self

    def attach(self, int key):
        """Attach existing shm segment with key and size"""
        self.new(key, 0, False)
        return self

    def detach(self):
        if self.seg is None:
            raise SHMCommonRuntimeError("Detach failed:  uninitialized object")

        try:
            _detach(self.seg)
        except Exception as e:
            raise SHMCommonRuntimeError(
                "Failed to detach shm object, key=%08x:  %s" %
                (self.seg.key,e))

        self.log.debug("Detached shm seg, key=%08x, size %d" %
                       (self._key, self._size))

    def unlink(self):
        if self.seg is None:
            raise SHMCommonRuntimeError("Unlink failed:  uninitialized object")

        try:
            _unlink(self.seg)
        except Exception as e:
            raise SHMCommonRuntimeError(
                "Failed to unlink shm object, key=%08x:  %s" %
                (self.seg.key,e))

        self.log.debug("Unlinked shm seg key=%08x" % self.seg.key)

def new(int key, int size):
    return shm_seg().new(key,size)

def attach(int key):
    return shm_seg().attach(key)

