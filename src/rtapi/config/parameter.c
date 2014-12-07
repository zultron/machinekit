/********************************************************************
 * Copyright (C) 2014 John Morris <john@zultron.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 ********************************************************************/

/*
 * Data structure:
 *
 * topsection		/				section_node
 *   sec1		/sec1				section_node
 *     subsec1		/sec1/subsec1			section_node
 *       key1		/sec1/subsec1, key1		parameter_node
 *         val1		/sec1/subsec1, key1[0] = val1	value_node
 *         val2		/sec1/subsec1, key1[1] = val2
 *       sub2sec1	/sec1/subsec1/sub2sec1
 *         key4		/sec1/subsec1/sub2sec1, key4
 *           val5	/sec1/subsec1/sub2sec1, key4[0] = val5
 *   sec2		/sec2
 *   key5		/, key5
 *     val6		/, key5[0] = val6
 *
 * bool, int, double and char* types are stored in a union in value_node;
 * char* bytes are in a separate malloc chunk
*/


#include <string.h>  // strcpy
//#include "rtapi_heap.h"
#include "rtapi_heap_private.h"  // need sizeof(rtapi_heap)
#include <assert.h>
// FIXME
#include <stdio.h>
#include "parameter.h"


/*
 * Config header and section, parameter and value node structures
 */
typedef struct rtapi_config_header {
    rtapi_heap heap;
    int rtapi_config_locked;
    size_t topsection_offset;
} rtapi_config_header;

typedef struct section_node {
    char name[RTAPI_CONFIG_NAME_MAX];
    size_t next;		// offset of next section in same level
    size_t child;		// offset of first subsection
    size_t parameter;		// offset of first parameter
} section_node;

typedef struct parameter_node {
    char name[RTAPI_CONFIG_NAME_MAX];
    size_t next_value;		// offset of next value_node
    size_t next_param;		// offset of next parameter
} parameter_node;

typedef struct value_node {
    union {			// union of all value types
	int vbool;
	int vint;
	double vdouble;
	size_t vstring;		// stored in chunk from rtapi_malloc()
    } value;
    size_t next;		// offset of next value_node
} value_node;

/*
 * Globals
 */
// Pointer to config tree header struct
static rtapi_config_header* header_ptr = NULL;
// Pointer to heap
static rtapi_heap* heap = NULL;
// Pointer to top section
static section_node* topsection_ptr = NULL;


/*
 * Node offset->ptr and ptr->offset functions for convenience
 */
static inline section_node* snode_off_to_ptr(size_t offset) {
    if (offset == 0) return NULL;
    return (section_node*)heap_ptr(heap, offset);
}
static inline parameter_node* pnode_off_to_ptr(size_t offset) {
    if (offset == 0) return NULL;
    return (parameter_node*)heap_ptr(heap, offset);
}
static inline value_node* vnode_off_to_ptr(size_t offset) {
    if (offset == 0) return NULL;
    return (value_node*)heap_ptr(heap, offset);
}
static inline size_t node_ptr_to_off(void* ptr) {
    return heap_off(heap, ptr);
}


/*
 * split_section_path():  Split top-level section name from a section path
 */
static const char* split_section_path(const char* section_path,
				      char* top_section_name)
{
    int i;

    // find the end of the top-level section name
    for (i=0; section_path[i] != '/' && section_path[i] != '\0';)
	i++;

    // copy substring result
    strncpy(top_section_name, section_path, i);
    top_section_name[i] = '\0';

    // if next character is a '/', bump past it
    if (section_path[i] == '/') i++;

    // return pointer to the beginning of the next subsection
    return (char*)section_path + i;
}


/*
 * Allocate, initialize and link a new section relative to a parent or
 * previous section node
 */
static int new_section(const char* name,
		       section_node* parent, section_node* prev)
{
    section_node* ptr;

    // Don't do anything if config is locked
    if (header_ptr->rtapi_config_locked == 1) {
	// FIXME
	printf("new_section:  config locked; returning 1\n");
	return 1;
    }
    // FIXME

    // Allocate space
    ptr = (section_node* )rtapi_malloc(heap, sizeof(section_node));
    if (ptr == NULL)
	return 1;

    // Set name and pointers
    strncpy(ptr->name, name, RTAPI_CONFIG_NAME_MAX);
    ptr->next = 0;
    ptr->child = 0;
    ptr->parameter = 0;

    // Set offsets in parent and prev, as applicable
    if (parent != NULL) {
	// FIXME
	printf("      created new section '%s'@%zu, parent '%s'@%zu\n",
	       ptr->name, node_ptr_to_off(ptr),
	       parent->name, node_ptr_to_off(parent));
	parent->child = node_ptr_to_off(ptr);
    }
    if (prev != NULL) {
	// FIXME
	printf("      created new section '%s'@%zu, prev '%s'@%zu\n",
	       ptr->name, node_ptr_to_off(ptr),
	       prev->name, node_ptr_to_off(prev));
	prev->next = node_ptr_to_off(ptr);
    }

    return 0;
}

/*
 * find_subsection(): Find and return the section_node described by the
 * section_path
 *
 * Call this with topsection_ptr as base, and a path like
 * '/sec/subsec'.  If the path doesn't exist, it will be created.
 */
static section_node* find_subsection(section_node* base,
				     const char* section_path)
{
    char section_name[RTAPI_CONFIG_NAME_MAX];
    const char* subsection_path;

    // extract base section name
    subsection_path = split_section_path(section_path, section_name);

    // FIXME
    printf("  find_subsection('%s'@%zu, %s)\n",
	   base->name, node_ptr_to_off(base), section_path);

    // Check if section name matches
    if (strcmp(base->name, section_name) == 0) {
	// If the section_path is exhausted, done
	if (subsection_path[0] == '\0') {
	    // FIXME
	    printf("MATCH Found section node '%s'@%zu\n",
		   section_name, node_ptr_to_off(base));
	    return base;
	}

	// Go deeper; create the subsection if needed and recurse
	if (base->child == 0) {
	    split_section_path(subsection_path, section_name);
	    if (new_section(section_name, base, NULL) == 1)
		return NULL;
	}
	// FIXME
	printf("    MATCH Searching subsection '%s'@%zu, path '%s'\n",
		snode_off_to_ptr(base->child)->name,
		base->child, subsection_path);
	return find_subsection(snode_off_to_ptr(base->child),
			       subsection_path);
    }

    // No section name match; create next section if needed and recurse
    if (base->next == 0) {
	if (new_section(section_name, NULL, base) == 1)
	    return NULL;
    }
    // FIXME
    printf("    Searching next section '%s'@%zu, path '%s'\n",
	    snode_off_to_ptr(base->next)->name,
	    base->next, subsection_path);
    return find_subsection(snode_off_to_ptr(base->next), section_path);
}


/*
 * Allocate, initialize and link a new parameter node
 */
static int new_parameter(const char* name, size_t* pnode_off_ptr)
{
    parameter_node* pnode;

    // Don't do anything if config is locked
    if (header_ptr->rtapi_config_locked == 1)
	return 1;

    // allocate space
    pnode = (parameter_node*)rtapi_malloc(heap, sizeof(parameter_node));
    if (pnode == NULL)
	return 1;

    // set name and offsets
    strncpy(pnode->name, name, RTAPI_CONFIG_NAME_MAX);
    pnode->next_value = 0;
    pnode->next_param = 0;

    // point prev offset here
    *pnode_off_ptr = node_ptr_to_off(pnode);

    return 0;
}


/*
 * find_parameter(): Locate a parameter in the config tree; create if
 * necessary
 */
static parameter_node* find_parameter(const char* section_path,
				      const char* name)
{
    section_node* subsection;
    size_t* pnode_off_ptr;

    // find subsection first
    if ((subsection = find_subsection(topsection_ptr, section_path)) == NULL)
	return NULL;

    // FIXME
    printf("  find_parameter('%s'/'%s')\n", section_path, name);

    // loop through parameter_nodes until found or list is empty
    for (pnode_off_ptr=&subsection->parameter;
	 *pnode_off_ptr != 0 &&
	     strcmp(pnode_off_to_ptr(*pnode_off_ptr)->name, name) != 0;) {
	// FIXME
	printf("    considering '%s'@%zu\n",
	       pnode_off_to_ptr(*pnode_off_ptr)->name, *pnode_off_ptr);
	pnode_off_ptr = &pnode_off_to_ptr(*pnode_off_ptr)->next_param;
    }

    // if list empty, create a parameter_node
    if (*pnode_off_ptr == 0) {
	// FIXME
	printf("      new_parameter('%s', %zu)\n", name, *pnode_off_ptr);
	if (new_parameter(name, pnode_off_ptr) == 1)
	    return NULL;
    }

    printf("MATCH Found parameter node '%s'@%zu\n", name, *pnode_off_ptr);
    return pnode_off_to_ptr(*pnode_off_ptr);
}

/*
 * new_value():  Allocate, initialize and link a new value node
 */
static int new_value(size_t* vnode_off_ptr)
{
    value_node* vnode;

    // Don't do anything if config is locked
    if (header_ptr->rtapi_config_locked == 1)
	return 1;

    // allocate space
    vnode = (value_node*)rtapi_malloc(heap, sizeof(value_node));
    if (vnode == NULL)
	return 1;

    // zero struct
    memset(vnode, 0, sizeof(value_node));

    // point prev offset here
    *vnode_off_ptr = node_ptr_to_off(vnode);

    // FIXME
    printf("      created new value @%zu/%p\n",
	   *vnode_off_ptr, vnode_off_to_ptr(*vnode_off_ptr));

    return 0;
}

/*
 * find_value(): find a value node in the config tree, create if
 * needed
 */
static value_node* find_value(const char* section_path, const char* name,
			      int index)
{
    parameter_node* parameter;
    size_t* vnode_off_ptr;
    int i;

    // find parameter first
    if ((parameter = find_parameter(section_path, name)) == NULL)
	return NULL;

    // FIXME
    printf("  find_value('%s'/'%s'[%d])\n", section_path, name, index);

    // loop through value_nodes until found or list is empty
    for (i=0, vnode_off_ptr=&parameter->next_value;
	 *vnode_off_ptr != 0 && i < index;) {
	// FIXME
	printf("    considering value [%d]@%zu\n", i, *vnode_off_ptr);
	vnode_off_ptr = &vnode_off_to_ptr(*vnode_off_ptr)->next;
	i++;
    }

    // if vnode_off == 0, create a value_node
    if (*vnode_off_ptr == 0) {
	// FIXME
	printf("      new_value([%d], %zu)\n", i, *vnode_off_ptr);
	if (new_value(vnode_off_ptr) == 1)
	    return NULL;
    }

    printf("MATCH Found value node %d@%zu\n", index, *vnode_off_ptr);
    return vnode_off_to_ptr(*vnode_off_ptr);
}

/*
 * value_node_iter_init():  Public:  Set up a value node iterator
 */
size_t rtapi_config_value_iter_init(const char* section_path, const char* name)
{
    return find_parameter(section_path, name)->next_value;
}

/*
 * rtapi_config_value_iter_next(): Return value node pointed to by
 * offset_ptr and update offset_ptr; used by the typed iter_next
 * functions below
 */
static value_node* rtapi_config_value_iter_next(size_t* offset_ptr)
{
    value_node* res = vnode_off_to_ptr(*offset_ptr);
    *offset_ptr = res->next;
	
    // FIXME
    printf("Next offset = %zu\n", *offset_ptr);
    return res;
}

/*
 * rtapi_config_check(): Public: Return true if a config parameter
 * exists (or can be created); used in Cython bindings
 */
int rtapi_config_check(const char* section_path, const char* name,
		       int index)
{
    return (find_value(section_path, name, index) != NULL);
}

/*
 * rtapi_config_bool(): Public: Return pointer to a config parameter's
 * boolean value for direct read/write access
 */
int* rtapi_config_bool(const char* section_path, const char* name,
		       int index)
{
    value_node* value;

    // find value node first
    if ((value = find_value(section_path, name, index)) == NULL)
	return NULL;

    // FIXME
    printf("  rtapi_config_bool('%s'/'%s'[%d])\n", section_path, name, index);

    return &value->value.vbool;
}

/*
 * rtapi_config_value_iter_next_bool(): Public: Return pointer to bool
 * value and update iter offset
 */
int* rtapi_config_value_iter_next_bool(size_t* offset_ptr)
{
    return &(rtapi_config_value_iter_next(offset_ptr))->value.vbool;
}

/*
 * rtapi_config_int(): Public: Return pointer to a config parameter's
 * integer value for direct read/write access
 */
int* rtapi_config_int(const char* section_path, const char* name,
		      int index)
{
    value_node* value;

    // find value node first
    if ((value = find_value(section_path, name, index)) == NULL)
	return NULL;

    // FIXME
    printf("  rtapi_config_int('%s'/'%s'[%d])\n", section_path, name, index);

    return &value->value.vint;
}

/*
 * rtapi_config_value_iter_next_int(): Public: Return pointer to int
 * value and update iter offset
 */
int* rtapi_config_value_iter_next_int(size_t* offset_ptr)
{
    return &rtapi_config_value_iter_next(offset_ptr)->value.vint;
}

/*
 * rtapi_config_double(): Public: Return pointer to a config parameter's
 * double value for direct read/write access
 */
double* rtapi_config_double(const char* section_path, const char* name,
			    int index)
{
    value_node* value;

    // find value node first
    if ((value = find_value(section_path, name, index)) == NULL)
	return NULL;

    // FIXME
    printf("  rtapi_config_double('%s'/'%s'[%d])\n", section_path, name, index);

    return &value->value.vdouble;
}

/*
 * rtapi_config_value_iter_next_double(): Public: Return pointer to
 * double value and update iter offset
 */
double* rtapi_config_value_iter_next_double(size_t* offset_ptr)
{
    return &rtapi_config_value_iter_next(offset_ptr)->value.vdouble;
}

/*
 * rtapi_config_string(): Public: Return pointer to a config
 * parameter's string value for direct read-only access; use
 * rtapi_config_string_set() for write
 */
char* rtapi_config_string(const char* section_path, const char* name,
			  int index)
{
    value_node* vnode;

    // find value node first
    if ((vnode = find_value(section_path, name, index)) == NULL)
	return NULL;

    // FIXME
    printf("  rtapi_config_string('%s'/'%s'[%d]) -> '%s'@%zu/%p\n",
	   section_path, name, index,
	   (char*)heap_ptr(heap, vnode->value.vstring), vnode->value.vstring,
	   heap_ptr(heap, vnode->value.vstring));

    // If the string malloc fails, the string offset will be 0
    if (vnode->value.vstring == 0)
	return NULL;
    return (char*)heap_ptr(heap, vnode->value.vstring);
}

/*
 * rtapi_config_value_iter_next_string(): Public: Return pointer to
 * string value and update iter offset
 */
char* rtapi_config_value_iter_next_string(size_t* offset_ptr)
{
    return heap_ptr(heap,
		    rtapi_config_value_iter_next(offset_ptr)->value.vstring);
}

/*
 * rtapi_config_string_set(): Public: Set a config parameter's string
 * value
 */
char* rtapi_config_string_set(const char* section_path, const char* name,
			      int index, const char* value)
{
    value_node* vnode;
    char* string;
    size_t old_string;

    // find value node first
    if ((vnode = find_value(section_path, name, index)) == NULL)
	return NULL;

    // malloc string;  FIXME if this fails, struct will be inconsistent
    if ((string = (char*)rtapi_malloc(heap, strlen(value))) == NULL)
	return NULL;
    strcpy(string, value);

    // update pointers
    old_string = vnode->value.vstring;
    vnode->value.vstring = node_ptr_to_off(string);
    if (old_string != 0)
    	rtapi_free(heap, heap_ptr(heap, old_string));

    // FIXME
    printf("  rtapi_config_string_set('%s'/'%s'[%d]) -> '%s'@%zu/%p\n",
	   section_path, name, index, string, vnode->value.vstring,
	   heap_ptr(heap,vnode->value.vstring));

    return string;
}


/*
 * rtapi_config_lock():  Lock the config tree structure
 *
 * When the config tree structure is locked, attempts to create new
 * sections, parameters or values will fail. Read/write access to the
 * parameters is unchanged.
 */
void rtapi_config_lock(int lock)
{
    header_ptr->rtapi_config_locked = lock;
}

/*
 * rtapi_config_attach():  Init global variables
 *
 * Before calling this, heap, *node_off_to_ptr() and node_ptr_to_off()
 * are unavailable
 */
int rtapi_config_attach(void* shm_ptr)
//size_t* tsop, rtapi_heap* h)
{
    header_ptr = (rtapi_config_header*)shm_ptr;
    heap = &header_ptr->heap;
    topsection_ptr = snode_off_to_ptr(header_ptr->topsection_offset);

    if (topsection_ptr == NULL)
	return 1;

    return 0;
}

/*
 * Given a shm segment, set up initial headers, top section node and
 * attach
 */
int rtapi_config_init(void* shm_ptr, int shm_size)
{
    int res;
    rtapi_config_header* local_header_ptr;	// local header pointer
    section_node* tsp;				// local pointer to top section
    void* arena_ptr;				// pointer to malloc arena
    int arena_size;				// size of malloc arena

    /*
     * Set up structures at top of shm segment and init allocator
     */

    // Lay down config header at top of shm segment
    local_header_ptr = (rtapi_config_header*)shm_ptr;
    memset(local_header_ptr, 0, sizeof(*local_header_ptr));

    // Init heap
    if ((res = rtapi_heap_init(&local_header_ptr->heap)) != 0) {
	// always returns 0
	return 1;
    }

    // Allocate heap arena in shm space following config header
    arena_ptr = (void*)(local_header_ptr + 1);
    arena_size = shm_size - sizeof(*local_header_ptr);
    res = rtapi_heap_addmem(&local_header_ptr->heap, arena_ptr, arena_size);
    if (res != 0) {
	// FIXME
	printf("Unable to add heap space\n");
	return 1;
    }
    // Sanity check:  arena ends at shm segment end
    assert(shm_ptr + shm_size == arena_ptr + arena_size);

    // FIXME
    printf("    shm = %p..%p; arena = %p..%p\n",
	   shm_ptr, (void *)shm_ptr+RTAPI_CONFIG_SHM_SIZE,
	   arena_ptr, (void *)arena_ptr + arena_size);

    /*
     * Set up the top section node
     */

    // Allocate, initialize and link the struct
    tsp = (section_node* )rtapi_malloc(&local_header_ptr->heap,
				       sizeof(section_node));
    if (tsp == NULL)
	return 1;
    memset(tsp, 0, sizeof(*tsp));
    local_header_ptr->topsection_offset =
	heap_off(&local_header_ptr->heap, tsp);

    // FIXME
    printf("Initialized tsp '%s'@%zu; child=%zu\n",
	   tsp->name, local_header_ptr->topsection_offset, tsp->child);

    /*
     * Attach config
     */

    // Init global heap and topsection_pointer variables
    if ((res = rtapi_config_attach(shm_ptr)) == 1) {
	// FIXME
	printf("Unable to init parameter structs\n");
	return 1;
    }
    // FIXME
    printf("   header_ptr=%p ?= shm_ptr=%p\n"
	   "   heap=%p ?= &local_header_ptr->heap=%p\n"
	   "   topsection_ptr=%zu/%p ?= tsp=%zu/%p\n"
	   "   ",
	   header_ptr, shm_ptr,
	   heap, &local_header_ptr->heap,
	   node_ptr_to_off(topsection_ptr), topsection_ptr,
	   node_ptr_to_off(tsp), tsp);

    // Sanity checks
    if (header_ptr != shm_ptr || heap != &local_header_ptr->heap
	|| topsection_ptr != tsp)
	return 1;

    return 0;
}
