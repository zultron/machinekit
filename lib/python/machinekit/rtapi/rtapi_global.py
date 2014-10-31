import rtapi_global_bindings
import logging

# import RTAPIGlobalDataException to pass along to other modules
# importing this one
from .rtapi_global_bindings import RTAPIGlobalDataException

class GlobalData(rtapi_global_bindings._GlobalData):
    def __init__(self, seg, config=None):
        self.seg = seg
        self.config = config
        self.log = logging.getLogger(self.__module__)

    def init_global_data(self):
        """One-time initialization of global data"""
        self.log.debug("Initializing global data:  instance %s, shm key %s",
                       self.config.instance, self.seg.key)

        # zero out global data segment
        self.zero()

        # obtain the mutex
        self.mutex_try()

        # lock global data segment into RAM
        self.mlock()

        # advertise state
        self.magic = self.GLOBAL_INITIALIZING

        # set version code for other modules to compare
        self.layout_version = self.GLOBAL_LAYOUT_VERSION
        self.instance_id = self.config.instance

        # set message levels
        self.rt_msg_level = self.config.rtapi_msglevel
        self.user_msg_level = self.config.ulapi_msglevel

        # counter for unique handles within an RTAPI instance
        #
        # guaranteed not to collide with a any module ID, so start at
        # RTAPI_MAX_MODULES + 1 (relevant for khreads); uthreads use
        # arbitrary ints since those dont use fixed-size arrays
        self.next_handle = self.RTAPI_MAX_MODULES + 1

        # tell the others what we determined as the proper flavor
        self.rtapi_thread_flavor = self.config.flavor_id

        # HAL segment size
        self.hal_size = self.config.hal_size

        # stack size passed to rtapi_task_new() in hal_create_thread()
        self.hal_thread_stack_size = self.config.hal_stack_size

        # set service_uuid
        self.service_uuid = self.config.mkuuid

        # init and attach the error ring
        self.error_ring_init()

        # demon pids
        self.rtapi_app_pid = -1;  # not yet started
        self.rtapi_msgd_pid = 0;

        # init rtapi heap and add memory
        self.rtapi_heap_init()

        # release the mutex
        self.mutex_give()
