cdef extern from "parameter.h":
    enum:
        RTAPI_CONFIG_NAME_MAX = 80
    enum:
        RTAPI_CONFIG_SHM_SIZE = 1024 * 100
    enum:
        RTAPI_CONFIG_SHM_KEY = 0x00BEEB00

    int rtapi_config_init(void *heam_shm_ptr, int size)
    int rtapi_config_attach(void* heap_shm_ptr)
    void rtapi_config_lock(int lock)

    int* rtapi_config_bool(
        const char *section, const char *name, int index)
    int* rtapi_config_int(
        const char *section, const char *name, int index)
    double* rtapi_config_double(
        const char *section, const char *name, int index)
    char* rtapi_config_string(
        const char *section, const char *name, int index)
    char* rtapi_config_string_set(
        const char *section, const char *name, int index, const char *value)
