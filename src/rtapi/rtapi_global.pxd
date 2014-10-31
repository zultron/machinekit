from .rtapi_heap cimport *  # struct rtapi_heap
from .ring cimport *

cdef extern from "rtapi_global.h":
    int GLOBAL_HEAP_SIZE
    int MESSAGE_RING_SIZE

    int GLOBAL_LAYOUT_VERSION

    int GLOBAL_INITIALIZING
    int GLOBAL_READY
    int GLOBAL_EXITED

    ctypedef struct global_data_t:
        unsigned magic
        int layout_version
        unsigned long mutex

        int instance_id
        char *instance_name
        int rtapi_thread_flavor

        int rt_msg_level
        int user_msg_level
        #rtapi_atomic_type next_handle
        int hal_size
        int hal_thread_stack_size

        unsigned char service_uuid[16]
        int rtapi_app_pid
        int rtapi_msgd_pid

        #rtapi_threadstatus_t thread_status[RTAPI_MAX_TASKS + 1]

        # int error_ring_full
        # int error_ring_locked

        ringheader_t rtapi_messages
        char buf[262144]   # This might change, but Cython doesn't
                           # know how to handle C macro definitions
        ringtrailer_t rtapi_messages_trailer

        rtapi_heap heap
        unsigned char *arena
