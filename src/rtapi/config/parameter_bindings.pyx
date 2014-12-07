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

    def __cinit__(self, object tree, bytes section, bytes name, int index,
                  uintptr_t ptr = 0):
        self.tree = tree
        self.section = section
        self.name = name
        self.index = index
        self._ptr = <void*>ptr
        # Raise any allocation-related exceptions during init; if a
        # pointer was passed, assume allocation has been done
        if ptr == 0 and \
                not rtapi_config_check(self.section, self.name, self.index):
            raise RTAPIParameterRuntimeError(
                "Unable to access parameter (%s,%s[%d])" %
                (self.section, self.name, self.index))

    cdef void* ptr(self):
        if self._ptr == NULL:
            self.set_ptr()
        return <void*>self._ptr

cdef class ParameterBool(Parameter):
    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_bool(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<bint*>self.ptr())[0]

        def __set__(self, object val):
            (<bint*>self.ptr())[0] = val

cdef class ParameterInt(Parameter):
    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_int(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<int*>self.ptr())[0]

        def __set__(self, object val):
            (<int*>self.ptr())[0] = val

cdef class ParameterDouble(Parameter):
    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_double(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<double*>self.ptr())[0]

        def __set__(self, object val):
            (<double*>self.ptr())[0] = val

cdef class ParameterString(Parameter):
    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_string(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            cdef char* p = <char*>self.ptr()
            if p == NULL:
                raise RTAPIParameterRuntimeError(
                    "Uninitialized string parameter (%s,%s[%d])" %
                    (self.section, self.name, self.index))
            return <char*>self.ptr()

        def __set__(self, object val):
            cdef char* p = rtapi_config_string_set(
                self.section, self.name, self.index, <bytes>val)
            if p == NULL:
                raise RTAPIParameterRuntimeError(
                    "Error setting string parameter (%s,%s[%d])" %
                    (self.section, self.name, self.index))
            self._ptr = <void*>p


cdef class ParameterIter:
    cdef object tree
    cdef bytes section
    cdef bytes name
    cdef object cls
    cdef int index
    cdef size_t next_offset

    def __cinit__(self, object tree,
                  bytes section, bytes name, object cls):
        self.tree = tree
        self.section = section
        self.name = name
        self.cls = cls
        self.index = -1
        self.next_offset = rtapi_config_value_iter_init(section, name)

    def __iter__(self):
        return self

    def __next__(self):
        if self.next_offset == 0:
            raise StopIteration()

        # Get value pointer
        cdef uintptr_t val_ptr
        if self.cls == ParameterBool:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_bool(
                &self.next_offset)
        elif self.cls == ParameterInt:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_int(
                &self.next_offset)
        elif self.cls == ParameterDouble:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_double(
                &self.next_offset)
        elif self.cls == ParameterString:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_string(
                &self.next_offset)

        # Increment index
        self.index += 1

        # Return Parameter* object
        return self.cls(
            self.tree, self.section, self.name, self.index, val_ptr)

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

    def booliter(self, bytes section, bytes name):
        return ParameterIter(self, section, name, ParameterBool)

    def intiter(self, bytes section, bytes name):
        return ParameterIter(self, section, name, ParameterInt)

    def doubleiter(self, bytes section, bytes name):
        return ParameterIter(self, section, name, ParameterDouble)

    def stringiter(self, bytes section, bytes name):
        return ParameterIter(self, section, name, ParameterString)

