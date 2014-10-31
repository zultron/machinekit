cdef extern from "Python.h":
    # Set error messages in exceptions from strerror
    ctypedef struct PyObject
    cdef PyObject *PyExc_IOError
    cdef PyObject *PyExc_MemoryError
    PyObject *PyErr_SetFromErrno(PyObject *)

cdef extern from "stdint.h" nogil:
    # uintptr_t is used to pass pointers around in python
    ctypedef size_t uintptr_t


# Numbers
cdef:
    ctypedef int int32
