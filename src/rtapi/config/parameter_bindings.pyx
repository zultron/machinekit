from .parameter cimport *
from cython_helpers cimport *
cimport cython


class RTAPIParameterRuntimeError(MemoryError):
    """
    Raised for errors in rtapi.config.parameter
    """
    pass

cdef class Parameter:
    cdef public object tree
    cdef public bytes section
    cdef public bytes name
    cdef public int index
    cdef void* _ptr

    def __cinit__(self, object tree,
                  bytes section, bytes name, int index):
        self.tree = tree
        self.section = section
        self.name = name
        self.index = index
        self._ptr = NULL

    cdef void* get_ptr(self):
        raise RTAPIParameterRuntimeError(
            "Subclasses of Parameter must override get_ptr()")

    cdef void* ptr(self):
        if self._ptr == NULL:
            self._ptr = self.get_ptr()
            if self._ptr == NULL:
                raise RTAPIParameterRuntimeError(
                    "Error retrieving parameter (%s,%s[%d])" %
                    (self.section, self.name, self.index))
        return <void*>self._ptr

cdef class ParameterBool(Parameter):
    cdef void* get_ptr(self):
        return <void*>rtapi_config_bool(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<bint*>self.ptr())[0]

        def __set__(self, object val):
            (<bint*>self.ptr())[0] = <bint>val

cdef class ParameterInt(Parameter):
    cdef void* get_ptr(self):
        return <void*>rtapi_config_int(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<int*>self.ptr())[0]

        def __set__(self, object val):
            (<int*>self.ptr())[0] = <int>val

cdef class ParameterDouble(Parameter):
    cdef void* get_ptr(self):
        return <void*>rtapi_config_double(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<double*>self.ptr())[0]

        def __set__(self, object val):
            (<double*>self.ptr())[0] = <double>val

cdef class ParameterString(Parameter):
    cdef void* get_ptr(self):
        return <void*>rtapi_config_string(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return <char*>self.ptr()

        def __set__(self, object val):
            cdef char* p = rtapi_config_string_set(
                self.section, self.name, self.index, <bytes>val)
            if p == NULL:
                raise RTAPIParameterRuntimeError(
                    "Error setting string parameter (%s,%s[%d])" %
                    (self.section, self.name, self.index))
            self._ptr = <void*>p


cdef class ParameterTree:
    cdef public object shm_seg
    cdef void* shm_seg_ptr

    def __cinit__(self, object shm_seg):
        self.shm_seg = shm_seg
        self.shm_seg_ptr = <void*><uintptr_t>shm_seg.ptr

    def init(self):
        cdef res = rtapi_config_init(self.shm_seg_ptr, self.shm_seg.size)
        if res == 1:
            raise RTAPIParameterRuntimeError(
                "Failed to init parameter shm segment")

    def attach(self):
        cdef res = rtapi_config_attach(self.shm_seg_ptr)
        if res == 1:
            raise RTAPIParameterRuntimeError(
                "Failed to attach parameter shm segment")

    def lock(self, bint unlock=0):
        rtapi_config_lock(not unlock)

    def boolp(self, bytes section, bytes name, int index=0):
        return ParameterBool(self, section, name, index)

    def intp(self, bytes section, bytes name, int index=0):
        return ParameterInt(self, section, name, index)

    def doublep(self, bytes section, bytes name, int index=0):
        return ParameterDouble(self, section, name, index)

    def stringp(self, bytes section, bytes name, int index=0):
        return ParameterString(self, section, name, index)

