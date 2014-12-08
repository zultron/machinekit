import errno as _errno
from .parameter cimport *
from cython_helpers cimport *
cimport cython
import os


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
        # pointer was passed, assume allocation has been done. Strings
        # are the exception; they're checked later.
        if ptr == 0 and self.val_type != RTAPI_CONFIG_TYPE_STRING and \
                not rtapi_config_check(self.section, self.name,
                                       self.index, self.val_type):
            if errno == _errno.EFAULT:
                raise RTAPIParameterRuntimeError(
                    "Error in %s: Bad index" % self)
            else:
                raise RTAPIParameterRuntimeError(
                    "Error in %s: %s [%d]" % (self, os.strerror(errno), errno))

    cdef void* ptr(self):
        if self._ptr == NULL:
            self.set_ptr()
        return <void*>self._ptr

    property type_str:
        def __get__(self):
            return rtapi_config_type_name[self.val_type]

    def __str__(self):
        return "<%s>(%s,%s[%d])" % \
            (self.type_str, self.section, self.name, self.index)

cdef class ParameterBool(Parameter):
    property val_type:
        def __get__(self):
            return RTAPI_CONFIG_TYPE_BOOL

    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_bool(
            self.section, self.name, self.index)
        if self._ptr == NULL:
            raise RTAPIParameterRuntimeError(
                "Error in %s:  %s [%d]" % (self, os.strerror(errno), errno))
        return None

    property val:
        def __get__(self):
            return (<bint*>self.ptr())[0]

        def __set__(self, object val):
            (<bint*>self.ptr())[0] = val

cdef class ParameterInt(Parameter):
    property val_type:
        def __get__(self):
            return RTAPI_CONFIG_TYPE_INT

    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_int(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<int*>self.ptr())[0]

        def __set__(self, object val):
            (<int*>self.ptr())[0] = val

cdef class ParameterDouble(Parameter):
    property val_type:
        def __get__(self):
            return RTAPI_CONFIG_TYPE_DOUBLE

    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_double(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            return (<double*>self.ptr())[0]

        def __set__(self, object val):
            (<double*>self.ptr())[0] = val

cdef class ParameterString(Parameter):
    property val_type:
        def __get__(self):
            return RTAPI_CONFIG_TYPE_STRING

    cpdef set_ptr(self):
        self._ptr = <void*>rtapi_config_string(
            self.section, self.name, self.index)

    property val:
        def __get__(self):
            cdef char* p = <char*>self.ptr()
            if p == NULL:
                if errno == _errno.EFAULT:
                    raise RTAPIParameterRuntimeError(
                        "Getting %s: Bad index" % self)
                else:
                    raise RTAPIParameterRuntimeError(
                        "Initializing %s: %s [%d]" % \
                            (self, os.strerror(errno), errno))
            return p

        def __set__(self, object val):
            cdef const char* p = rtapi_config_string_set(
                self.section, self.name, self.index, <bytes>val)
            if p == NULL:
                if errno == _errno.EFAULT:
                    raise RTAPIParameterRuntimeError(
                        "Setting %s: Bad index" % self)
                else:
                    raise RTAPIParameterRuntimeError(
                        "Initializing %s: %s [%d]" % \
                            (self, os.strerror(errno), errno))
            self._ptr = <void*>p


cdef class ParameterValueIter:
    cdef object tree
    cdef bytes section
    cdef bytes name
    cdef rtapi_config_type val_type
    cdef int index
    cdef size_t next_offset

    def __cinit__(self, object tree,
                  bytes section, bytes name, rtapi_config_type val_type):
        self.tree = tree
        self.section = section
        self.name = name
        self.val_type = val_type
        self.index = -1
        self.next_offset = rtapi_config_value_iter_init(section, name)

    def __iter__(self):
        return self

    def __next__(self):
        if self.next_offset == 0:
            raise StopIteration()

        # Get value pointer
        cdef uintptr_t val_ptr
        cdef object cls
        if self.val_type == RTAPI_CONFIG_TYPE_BOOL:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_bool(
                &self.next_offset)
            cls = ParameterBool
        elif self.val_type == RTAPI_CONFIG_TYPE_INT:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_int(
                &self.next_offset)
            cls = ParameterInt
        elif self.val_type == RTAPI_CONFIG_TYPE_DOUBLE:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_double(
                &self.next_offset)
            cls = ParameterDouble
        elif self.val_type == RTAPI_CONFIG_TYPE_STRING:
            val_ptr = <uintptr_t>rtapi_config_value_iter_next_string(
                &self.next_offset)
            cls = ParameterString

        # Increment index
        self.index += 1

        # Return Parameter* object
        return cls(self.tree, self.section, self.name, self.index, val_ptr)

cdef class ParameterIter:
    cdef object tree
    cdef bytes section
    cdef size_t next_offset

    def __cinit__(self, object tree, bytes section):
        self.tree = tree
        self.section = section
        self.next_offset = rtapi_config_parameter_iter_init(section)

    def __iter__(self):
        return self

    def __next__(self):
        if self.next_offset == 0:
            raise StopIteration()

        cdef rtapi_config_type val_type
        cdef const char* name = rtapi_config_parameter_iter_next(
            &self.next_offset, &val_type)

        # Return parameter name and next offset
        return (name, val_type)


cdef class SubsectionIter:
    cdef object tree
    cdef bytes section
    cdef size_t next_offset

    def __cinit__(self, object tree, bytes section):
        self.tree = tree
        self.section = section
        self.next_offset = rtapi_config_subsection_iter_init(section)

    def __iter__(self):
        return self

    def __next__(self):
        if self.next_offset == 0:
            raise StopIteration()

        # Return parameter name and next offset
        return rtapi_config_subsection_iter_next(&self.next_offset)


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
        return ParameterValueIter(self, section, name, RTAPI_CONFIG_TYPE_BOOL)

    def intiter(self, bytes section, bytes name):
        return ParameterValueIter(self, section, name, RTAPI_CONFIG_TYPE_INT)

    def doubleiter(self, bytes section, bytes name):
        return ParameterValueIter(self, section, name, RTAPI_CONFIG_TYPE_DOUBLE)

    def stringiter(self, bytes section, bytes name):
        return ParameterValueIter(self, section, name, RTAPI_CONFIG_TYPE_STRING)

    def valueiter(self, bytes section, bytes name, rtapi_config_type val_type):
        return ParameterValueIter(self, section, name, val_type)

    def paramiter(self, bytes section):
        return ParameterIter(self, section)

    def subsectioniter(self, bytes section):
        return SubsectionIter(self, section)

