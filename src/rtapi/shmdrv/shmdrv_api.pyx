# cython: binding=True 

from .shmdrv_api cimport *

import os

#############################
# common class attributes

def init():
    r = c_shm_common_init() # always returns 0

class SHMDrvAPIRuntimeError(RuntimeError):
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
        raise SHMDrvAPIRuntimeError(
            "Accessing shmdrv device node: permission denied")
    elif res != 0:
        raise SHMDrvAPIRuntimeError(
            "Accessing shmdrv device node: %s [%s]" %
            (os.strerror(res), res))

def shmdrv_loaded():
    return c_shmdrv_loaded != 0

#############################
# POSIX wrappers

cdef assert_shm_name_length_consistency():
    """
    assert consistent SHM_NAME length constants in shmdrv.h and
    shmdrv_api.pxd
    """
    if SHM_NAME_MAXLEN != LOCAL_SHM_NAME_MAXLEN:
        raise SHMDrvAPIRuntimeError(
            "inconsistent SHM_NAME_MAXLEN values in "
            "shmdrv.h and shmdrv.pxd")
    if SHM_NAME_PREFIX_MAXLEN != LOCAL_SHM_NAME_PREFIX_MAXLEN:
        raise SHMDrvAPIRuntimeError(
            "inconsistent SHM_NAME_PREFIX_MAXLEN values in "
            "shmdrv.h and shmdrv.pxd")
    
def set_name_format(str prefix):
    """set the POSIX shm name format"""
    assert_shm_name_length_consistency()
    if len(prefix) > SHM_NAME_PREFIX_MAXLEN:
        raise SHMDrvAPIRuntimeError(
            "shm prefix length exceeds maximum:  %d > %d" %
            (len(prefix), SHM_NAME_PREFIX_MAXLEN))
    c_shm_common_set_name_format(prefix)

def exists(int key):
    cdef object result = <bint> c_shm_common_exists(key)
    return result

#############################
# shmdrv_api C wrapper

cdef class SHMSegment:
    cdef void *_ptr
    cdef public int key
    cdef public int size

    def __repr__(self):
        return "<shm segment %s>" % self.key
    def __cinit__(self):
        self.key = 0
        self.size = 0
        self._ptr = NULL
        # Be sure that the value in shmdrv.h and here haven't diverged
        assert_shm_name_length_consistency()

    property posix_name:
        def __get__(self):
            if self.key == 0:  return None

            assert_shm_name_length_consistency()
            cdef char segment_name[LOCAL_SHM_NAME_MAXLEN]
            c_shm_common_segment_posix_name(segment_name, self.key)
            cpdef bytes res = segment_name
            return res

    property ptr:
        def __get__(self): return <uintptr_t>self._ptr


    cdef _wrap_exception(self, str msg):
        try:  # Put strerror string into our own exception type
            PyErr_SetFromErrno(PyExc_MemoryError)
        except Exception as e:
            raise SHMDrvAPIRuntimeError(msg % e)


    def new(self, int key, int size, bint create = 1):
        cdef int res
        cdef bytes oper = <bytes>(('attach','create')[create])

        if key == 0:
            raise SHMDrvAPIRuntimeError(
                "Refusing to %s segment: requested key=%08x" %
                (oper, self.key))

        if self.key != 0:
            raise SHMDrvAPIRuntimeError(
                "Failed to %s segment: object already initialized: "
                "key=%08x" % (oper, self.key))

        self.key = key
        self.size = size

        # get new shm seg
        res = c_shm_common_new(self.key, &self.size, &self._ptr, create)
        if res < 0:
            raise SHMDrvAPIRuntimeError(
                "Failed to %s shm object, key=%08x size=%d:  %s" %
                (("attach","create")[create], key, self.size,
                 os.strerror(-res)))

        if create == 1 and res == 0:
            raise SHMDrvAPIRuntimeError(
                "Failed to create new shm object:  key=%08x already exists" %
                key)

        return self  # so we can e.g. SHMSegment('global').new().unlink()
        
    def attach(self, int key):
        """Attach existing shm segment with key and size"""
        SHMSegment.new(self, key, 0, 0)
        return self

    def exists(self):
        """Check if shm segment exists"""
        return exists(self.key)

    def detach(self):
        """Detach existing shm segment"""
        if self.key == 0:
            raise SHMDrvAPIRuntimeError("Detach failed:  uninitialized object")

        cdef int res = c_shm_common_detach(self.size, self._ptr)
        if res == -1:
            self._wrap_exception(
                "Failed to detach shm object, key=%08x:  %%s" % self.key)

    def unlink(self):
        if self.key == 0:
            raise SHMDrvAPIRuntimeError("Unlink failed:  uninitialized object")

        cdef int res = c_shm_common_unlink(self.key)
        if res == -1:
            self._wrap_exception(
                "Failed to unlink shm object, key=%08x:  %%s" % self.key)
