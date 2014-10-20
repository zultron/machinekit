from .shmcommon cimport *
from .global_data cimport *
#from os import strerror

def shmdrv_available():
    return c_shmdrv_available()

def shmdrv_loaded():
    return c_shmdrv_loaded != 0

def shm_common_exists(int segment):
    return c_shm_common_exists(segment)

def shm_common_init():
    r = c_shm_common_init()
    # always returns 0
