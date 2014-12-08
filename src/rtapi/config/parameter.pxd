cdef extern from "parameter.h":
    # Constants, enums, variables
    enum:
        RTAPI_CONFIG_NAME_MAX = 80
    enum:
        RTAPI_CONFIG_SHM_SIZE = 1024 * 100
    enum:
        RTAPI_CONFIG_SHM_KEY = 0x00BEEB00
    ctypedef enum rtapi_config_type:
        RTAPI_CONFIG_TYPE_ANY,
        RTAPI_CONFIG_TYPE_BOOL,
        RTAPI_CONFIG_TYPE_INT,
        RTAPI_CONFIG_TYPE_DOUBLE,
        RTAPI_CONFIG_TYPE_STRING
    const char** rtapi_config_type_name

    # Initializers
    int rtapi_config_init(void *heam_shm_ptr, int size)
    int rtapi_config_attach(void* heap_shm_ptr)
    void rtapi_config_lock(int lock)

    # Accessors
    int rtapi_config_check(
        const char* section, const char* name, int index,
        rtapi_config_type val_type)
    int* rtapi_config_bool(
        const char *section, const char *name, int index)
    int* rtapi_config_int(
        const char *section, const char *name, int index)
    double* rtapi_config_double(
        const char *section, const char *name, int index)
    const char* rtapi_config_string(
        const char *section, const char *name, int index)
    const char* rtapi_config_string_set(
        const char *section, const char *name, int index, const char *value)

    # Iterators
    size_t rtapi_config_value_iter_init(const char* section_path,
                                        const char* name)
    int* rtapi_config_value_iter_next_bool(size_t* offset_ptr)
    int* rtapi_config_value_iter_next_int(size_t* offset_ptr)
    double* rtapi_config_value_iter_next_double(size_t* offset_ptr)
    const char* rtapi_config_value_iter_next_string(size_t* offset_ptr)

    size_t rtapi_config_parameter_iter_init(const char* section_path)
    const char* rtapi_config_parameter_iter_next(size_t* offset_ptr,
                                                 rtapi_config_type* vtype)

    size_t rtapi_config_subsection_iter_init(const char* section_path)
    const char* rtapi_config_subsection_iter_next(size_t* offset_ptr)
