# vim: sts=4 sw=4 et
cdef extern from "stdint.h" nogil:
    ctypedef size_t uintptr_t

cdef extern from "errno.h":
    cdef int errno

cdef extern from "Python.h":
    ctypedef struct PyObject
    cdef PyObject *PyExc_MemoryError
    PyObject *PyErr_SetFromErrno(PyObject *)

cdef extern from "shmdrv.h":
    cdef const int SHM_NAME_PREFIX_MAXLEN
    cdef const int SHM_NAME_MAXLEN

    cdef extern int c_shmdrv_loaded "shmdrv_loaded"

    ctypedef struct shm_stat:
        int driver_fd
        int  key
        size_t size
        size_t act_size
        void *addr
        int flags
        int id
        int n_kattach
        int n_uattach
        int creator
        int shmdrv_loaded

    int c_shmdrv_available "shmdrv_available"()
    # int c_shmdrv_driver_fd "shmdrv_driver_fd"()
    # int c_shmdrv_status "shmdrv_status"(shm_status *shmstat)
    # int c_shmdrv_create "shmdrv_create"(shm_status *shmstat)
    # int c_shmdrv_attach "shmdrv_attach"(shm_status *shmstat, void **shm)
    # int c_shmdrv_detach "shmdrv_detach" (shm_status *shmstat, void *shm)
    int c_shmdrv_gc "shmdrv_gc"()
    # void c_shmdrv_print_status "shmdrv_print_status" (shm_status *sm, const char *tag)

    void c_shm_common_set_name_format "shm_common_set_name_format"(
        const char *prefix)
    void c_shm_common_segment_posix_name "shm_common_segment_posix_name"(
        char *segment_name, int key)

    int c_shm_common_init "shm_common_init"()
    int c_shm_common_new "shm_common_new" (int key, int *size,
                                           void **shmptr, int create)
    int c_shm_common_detach "shm_common_detach" (int size, void *shmptr)
    bint c_shm_common_exists "shm_common_exists" (int key)
    int c_shm_common_unlink "shm_common_unlink" (int key)


# Cython can't handle a C #define specially, but only as an int, so
# this doesn't work: cdef char foo[SHM_NAME_MAXLEN]; instead, an enum
# has similar properties, and consistency is checked in functions
# where it's used
cdef enum:
    LOCAL_SHM_NAME_PREFIX_MAXLEN = 128
cdef enum:
    LOCAL_SHM_NAME_MAXLEN = 136


cdef struct _shm_seg_struct:
    int _key
    int _size
    void *_ptr
