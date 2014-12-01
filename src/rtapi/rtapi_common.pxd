cdef extern from "rtapi_common.h":

    # sizeof(rtapi_data_t) needed for shm segment allocation
    ctypedef struct rtapi_data_t:
        pass
