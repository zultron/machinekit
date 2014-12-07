#ifndef _RTAPI_CONFIG_PARAMETER_INCLUDED
#define _RTAPI_CONFIG_PARAMETER_INCLUDED

#include <stddef.h>  // size_t
#include "rtapi/rtapi_heap.h"  // rtapi_heap

// Max section name length
#define RTAPI_CONFIG_NAME_MAX 80
// Default parameter shm size
#define RTAPI_CONFIG_SHM_SIZE 1024 * 100
// Parameter shm magic
#define RTAPI_CONFIG_SHM_KEY 0x00BEEB00

// FIXME
extern void testy(void);

// Set the root section and heap pointers
/* extern int rtapi_config_init(size_t *topsection_offset_ptr, */
/* 				rtapi_heap *heap); */
extern int rtapi_config_init(void* heap_shm_ptr, int shm_size);

// Initialize a new root section
/* extern int rtapi_paramater_new(void); */
extern int rtapi_config_attach(void* heap_shm_ptr);

// Lock the config to structural changes; 0 is unlocked, 1 is locked
extern void rtapi_config_lock(int lock);

// Parameter value pointers
//
// Parameters are referenced by a key, (`section/subsection/...`, name),
// and may have multiple values accessed by an integer `index`.
//
// Returns a pointer to the value on success, NULL on failure
extern int rtapi_config_check(const char *section, const char *name,
			      int index);
extern int* rtapi_config_bool(const char *section, const char *name,
			      int index);
extern int* rtapi_config_int(const char *section, const char *name,
			     int index);
extern double* rtapi_config_double(const char *section, const char *name,
				   int index);
// Strings are dynamically allocated, and so need separate get and set
// accessors.
extern char* rtapi_config_string(const char *section, const char *name,
				 int index);
extern char* rtapi_config_string_set(const char *section, const char *name,
				     int index, const char *value);

// Parameter value iterators
//
// Functions for walking through multi-value parameters
extern size_t rtapi_config_value_iter_init(const char* section_path,
					   const char* name);
extern int* rtapi_config_value_iter_next_bool(size_t* offset_ptr);
extern int* rtapi_config_value_iter_next_int(size_t* offset_ptr);
extern double* rtapi_config_value_iter_next_double(size_t* offset_ptr);
extern char* rtapi_config_value_iter_next_string(size_t* offset_ptr);

#endif // _RTAPI_CONFIG_PARAMETER_INCLUDED
