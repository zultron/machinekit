cdef extern from "rtapi.h":

    # MUTEX FUNCTIONS
    void rtapi_mutex_give(unsigned long *mutex)
    int rtapi_mutex_try(unsigned long *mutex)
    void rtapi_mutex_get(unsigned long *mutex)
