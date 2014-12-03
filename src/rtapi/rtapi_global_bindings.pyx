from .cython_helpers cimport *
from .rtapi cimport *
from .rtapi_global cimport *
from .rtapi_heap cimport *
from .ring cimport *
from .mk_config cimport *

import os

cdef extern from "string.h":
    char *strncpy(char *dest, const char *src, size_t n)
    void *memset(void *s, int c, size_t n)

cdef extern from "sys/mman.h":
    int mlock(const void *addr, size_t len)
    int munlock(const void *addr, size_t len)

cdef extern from "uuid/uuid.h":
    ctypedef unsigned char uuid_t[16]
    int uuid_parse(const char *instr, uuid_t uu)
    void uuid_unparse(const uuid_t uu, char *outstr)


class RTAPIGlobalDataException(RuntimeError):
    """Raised for exceptions with global_data segment"""
    pass


cdef class _GlobalData:
    cdef global_data_t *_ptr
    cdef ringbuffer_t rtapi_msg_buffer
    cdef char uuid_buf[16]
    cdef char uuid_str_buf[37]
    cdef object segment

    def __cinit__(self, object seg, object config=None):
        self.segment = seg
        cdef uintptr_t i = seg.ptr  # can't cast and assign directly
                                    # to self._ptr
        self._ptr = <global_data_t *>i

    # macros from rtapi_global
    property GLOBAL_HEAP_SIZE:
        def __get__(self):
            return GLOBAL_HEAP_SIZE

    property MESSAGE_RING_SIZE:
        def __get__(self):
            return MESSAGE_RING_SIZE

    property GLOBAL_LAYOUT_VERSION:
        def __get__(self):
            return GLOBAL_LAYOUT_VERSION

    property GLOBAL_INITIALIZING:
        def __get__(self):
            return GLOBAL_INITIALIZING

    property GLOBAL_READY:
        def __get__(self):
            return GLOBAL_READY

    property GLOBAL_EXITED:
        def __get__(self):
            return GLOBAL_EXITED

    # macros from config.h (rtapi/mk_config.pxd)
    property RTAPI_MAX_MODULES:
        def __get__(self):
            return RTAPI_MAX_MODULES

    # properties from global_data_t struct
    property ptr:  # mostly for debugging
        def __get__(self):
            return <uintptr_t>self._ptr

    property magic:
        def __get__(self):
            return self._ptr.magic
        def __set__(self, int magic):
            self._ptr.magic = magic

    property layout_version:
        def __get__(self):
            return self._ptr.layout_version
        def __set__(self, int layout_version):
            self._ptr.layout_version = layout_version

    property mutex:
        def __get__(self):
            return self._ptr.mutex

    property instance_id:
        def __get__(self):
            return self._ptr.instance_id
        def __set__(self, int instance_id):
            self._ptr.instance_id = instance_id

    property rtapi_thread_flavor:
        def __get__(self):
            return self._ptr.rtapi_thread_flavor
        def __set__(self, int rtapi_thread_flavor):
            self._ptr.rtapi_thread_flavor = rtapi_thread_flavor

    property rt_msg_level:
        def __get__(self):
            return self._ptr.rt_msg_level
        def __set__(self, int rt_msg_level):
            self._ptr.rt_msg_level = rt_msg_level

    property user_msg_level:
        def __get__(self):
            return self._ptr.user_msg_level
        def __set__(self, int user_msg_level):
            self._ptr.user_msg_level = user_msg_level

    property hal_size:
        def __get__(self):
            return self._ptr.hal_size
        def __set__(self, int hal_size):
            self._ptr.hal_size = hal_size

    property hal_thread_stack_size:
        def __get__(self):
            return self._ptr.hal_thread_stack_size
        def __set__(self, int hal_thread_stack_size):
            self._ptr.hal_thread_stack_size = hal_thread_stack_size

    property service_uuid:
        def __get__(self):
            cdef char uuid[37]
            uuid_unparse(<uuid_t>self._ptr.service_uuid, <char *>uuid)
            cdef bytes uuid_out = uuid
            return uuid_out
        def __set__(self, bytes service_uuid):
            cdef char uuid_buf[37]
            strncpy(uuid_buf, service_uuid, 37)
            if uuid_parse(<char *>uuid_buf,
                           <uuid_t>(self._ptr.service_uuid)) == -1:
                raise RTAPIGlobalDataException(
                    "Unable to parse service UUID string '%s'" % service_uuid)

    property rtapi_app_pid:
        def __get__(self):
            return self._ptr.rtapi_app_pid
        def __set__(self, int rtapi_app_pid):
            self._ptr.rtapi_app_pid = rtapi_app_pid

    property rtapi_msgd_pid:
        def __get__(self):
            return self._ptr.rtapi_msgd_pid
        def __set__(self, int rtapi_msgd_pid):
            self._ptr.rtapi_msgd_pid = rtapi_msgd_pid

    # Helpers to manipulate stuff not possible in python

    def zero(self):
        memset(self._ptr, 0, sizeof(global_data_t))

    # FIXME these go elsewhere; rtapi?
    def mutex_try(self):
        if rtapi_mutex_try(&(self._ptr.mutex)) == 1:
            raise RTAPIGlobalDataException(
                "Unable to acquire global data mutex")

    def mutex_give(self):
        rtapi_mutex_give(&(self._ptr.mutex))

    # FIXME these go elsewhere; shmdrv_api?  utils?
    def mlock(self):
        if mlock(<void *>self._ptr, sizeof(global_data_t)) == -1:
            raise RTAPIGlobalDataException(
                "Failed to mlock global data segment at 0x%x, size %s: %s" %
                (self.ptr, sizeof(global_data_t), os.strerror(errno)), errno)

    def munlock(self):
        if munlock(<void *>self._ptr, sizeof(global_data_t)) == -1:
            raise RTAPIGlobalDataException(
                "Failed to munlock global data segment at 0x%x, size %s: %s" %
                (self.ptr, sizeof(global_data_t), os.strerror(errno)), errno)

    # FIXME this goes elsewhere; ring?  msg_buffer?
    def rtapi_msg_buffer_init(self):
        ringheader_init(&(self._ptr.rtapi_messages), 0, MESSAGE_RING_SIZE, 0)
        ringbuffer_zero(&(self._ptr.rtapi_messages))

        ringbuffer_init(&(self._ptr.rtapi_messages),
                         &(self.rtapi_msg_buffer))
        self.rtapi_msg_buffer.header.refcount = 1
        self.rtapi_msg_buffer.header.reader = os.getpid();
        self._ptr.rtapi_messages.use_wmutex = 1

    def rtapi_msg_buffer_cleanup(self):
        if self.rtapi_msg_buffer.header != NULL:
            self.rtapi_msg_buffer.header.refcount -= 1
