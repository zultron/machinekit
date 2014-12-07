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

#include <string.h>  // strcpy
//#include "rtapi_heap.h"
#include "rtapi_heap_private.h"  // need sizeof(rtapi_heap)
#include <assert.h>
// FIXME
#include <stdio.h>

#include "parameter.h"

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

// Pointer to heap
static rtapi_heap* heap = NULL;
static int rtapi_config_locked = 0;

typedef struct rtapi_config_header {
    rtapi_heap heap;
    int rtapi_config_locked;
    size_t topsection_offset;
} rtapi_config_header;
static rtapi_config_header* header_ptr;


/*
 * Section, parameter and value node structures
 *
 * Node offset->ptr and ptr->offset functions for convenience
 */
typedef struct section_node {
    char name[RTAPI_CONFIG_NAME_MAX];
    size_t next;		// offset of next section in same level
    size_t child;		// offset of first subsection
    size_t parameter;		// offset of first parameter
} section_node;
static inline section_node* snode_off_to_ptr(size_t offset) {
    return (section_node*)heap_ptr(heap, offset);
}

typedef struct parameter_node {
    char name[RTAPI_CONFIG_NAME_MAX];
    size_t next_value;		// offset of next value_node
    size_t next_param;		// offset of next parameter
} parameter_node;
static inline parameter_node* pnode_off_to_ptr(size_t offset) {
    return (parameter_node*)heap_ptr(heap, offset);
}

typedef struct value_node {
    union {			// union of all value types
	int vbool;
	int vint;
	double vdouble;
	size_t vstring;		// stored in chunk from rtapi_malloc()
    } value;
    size_t next;		// offset of next value_node
} value_node;
static inline value_node* vnode_off_to_ptr(size_t offset) {
    return (value_node*)heap_ptr(heap, offset);
}

static inline size_t node_ptr_to_off(void* ptr) {
    return heap_off(heap, ptr);
}

static section_node* topsection_ptr = NULL;

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
 * Create a new section relative to a parent or previous section node
 */
static int new_section(const char* name,
		       section_node* parent, section_node* prev)
{
    section_node* ptr;

    // Don't do anything if config is locked
    if (rtapi_config_locked == 1) {
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


static int new_parameter(const char* name, size_t* pnode_off_ptr)
{
    parameter_node* pnode;

    // Don't do anything if config is locked
    if (rtapi_config_locked == 1)
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

static int new_value(size_t* vnode_off_ptr)
{
    value_node* vnode;

    // Don't do anything if config is locked
    if (rtapi_config_locked == 1)
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
    printf("      created new value @%zu\n", *vnode_off_ptr);

    return 0;
}

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

char* rtapi_config_string(const char* section_path, const char* name,
			  int index)
{
    value_node* vnode;

    // find value node first
    if ((vnode = find_value(section_path, name, index)) == NULL)
	return NULL;

    // FIXME
    printf("  rtapi_config_string('%s'/'%s'[%d]) -> '%s'@%zu\n",
	   section_path, name, index,
	   (char*)heap_ptr(heap, vnode->value.vstring), vnode->value.vstring);

    // If the string malloc fails, the string offset will be 0
    if (vnode->value.vstring == 0)
	return NULL;
    return (char*)heap_ptr(heap, vnode->value.vstring);
}

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
    printf("  rtapi_config_string_set('%s'/'%s'[%d]) -> '%s'@%zu\n",
	   section_path, name, index, string, vnode->value.vstring);

    return string;
}

void rtapi_config_lock(int lock)
{
    rtapi_config_locked = lock;
}

void testy(void)
{
    char** section_list = (char *[]){
	"/sec1/subsec1/subsubsec1",
	"/sec1/subsec1/subsubsec2",
	"/sec1/subsec2/subsubsec3",
	/* "/sec1/subsec3", */
	/* "/sec2", */
	/* "/sec2/subsec4", */
	/* "/sec1/subsec1", */
	"/sec1/subsec2/subsubsec3",
	"",
	NULL
    };
    char** param_list = (char *[]){
	"param1",
	"param2",
	"param1",
	/* "param2", */
	NULL
    };
    char** param_list_ptr;
    int* int_list = (int []){5, 0, 1, 2, 1, 0, 2, -1};
    int* int_list_ptr;
    value_node* value;
    
    /* rtapi_config_lock(1); */

    for (; section_list[0] != NULL; section_list++) {
	for (param_list_ptr = param_list; param_list_ptr[0] != NULL;
	     param_list_ptr++) {
	    for (int_list_ptr = int_list; int_list_ptr[0] != -1;
		 int_list_ptr++) {
		printf("\nfind_value (%s/%s[%d])\n",
		       section_list[0], param_list_ptr[0], int_list_ptr[0]);
		value = find_value(section_list[0], param_list_ptr[0],
				   int_list_ptr[0]);
		if (value == NULL) {
		    printf("Got NULL result\n");
		    return;
		}
		printf("Found value [%d]@%zu, offsets next = %zu\n",
		       int_list_ptr[0], node_ptr_to_off(value), value->next);
	    }
	}
    }
}

void testy_next_subsection(void)
{
    const char* section_path = "/sec/subsec/subsubsec";
    char section_name[RTAPI_CONFIG_NAME_MAX];
    int i;
    
    for (i=0; i<=4; i++) {
    	section_path = split_section_path(section_path, section_name);
    	printf("result: %s\n", section_name);
    }
}

static section_node* get_topsection_ptr(void* heap_shm_ptr)
{
    size_t* topsection_off_ptr;

    // Offset of top section is located after heap struct
    topsection_off_ptr = heap_shm_ptr + sizeof(rtapi_heap);

    // Be sure top parameter section is initialized
    if (*topsection_off_ptr == 0)
	return NULL;

    // Return ptr to top section node
    return snode_off_to_ptr(*topsection_off_ptr);
}

int rtapi_config_attach(void* heap_shm_ptr)
//size_t* tsop, rtapi_heap* h)
{
    heap = heap_shm_ptr;
    topsection_ptr = get_topsection_ptr(heap_shm_ptr);

    if (topsection_ptr == NULL)
	return 1;

    return 0;
}

int rtapi_config_init(void* heap_shm_ptr, int shm_size)
{
    int res;
    size_t* tsop;		// pointer to top section offset
    section_node* tsp;		// local pointer to top section
    void* arena_ptr;		// pointer to malloc arena
    int arena_size;		// size of malloc arena

    /*
     * Set up structures at top of shm segment and init allocator
     */

    // Lay down heap struct at beginning of shm segment
    if ((res = rtapi_heap_init(heap_shm_ptr)) != 0) {
	// always returns 0
	return 1;
    }

    // Offset of top section after heap struct
    tsop = heap_shm_ptr + sizeof(rtapi_heap);
    *tsop = 0;

    // Allocate heap arena in shm space following top section offset
    arena_ptr = (void*)tsop + sizeof(*tsop);
    arena_size = shm_size -
	(sizeof(rtapi_heap) + sizeof(*tsop));
    if ((res = rtapi_heap_addmem(heap_shm_ptr, arena_ptr, arena_size)) != 0) {
	// FIXME
	printf("Unable to allocate heap space\n");
	return 1;
    }
    // Sanity check:  arena ends at shm segment end
    assert(heap_shm_ptr + shm_size == (void *)arena_ptr + arena_size);

    // FIXME
    printf("    shm = %p..%p; arena = %p..%p\n",
	   heap_shm_ptr, (void *)heap_shm_ptr+RTAPI_CONFIG_SHM_SIZE,
	   arena_ptr, (void *)arena_ptr + arena_size);

    /*
     * Set up the top section node
     */

    // Allocate the struct
    tsp = (section_node* )rtapi_malloc((rtapi_heap*)heap_shm_ptr,
				       sizeof(section_node));
    if (tsp == NULL)
	return 1;

    // The top section node offset is the handle
    *tsop = heap_off((rtapi_heap*)heap_shm_ptr, tsp);

    // Set empty name and null pointers
    strcpy(tsp->name, "");
    tsp->next = 0;
    tsp->child = 0;
    tsp->parameter = 0;

    // FIXME
    printf("Initialized tsp '%s'@%zu; child=%zu\n",
	   tsp->name,
	   heap_off((rtapi_heap*)heap_shm_ptr, tsp), tsp->child);

    /*
     * Init global variables
     */

    // Init global heap and topsection_pointer variables
    if ((res = rtapi_config_attach(heap_shm_ptr)) == 1) {
	// FIXME
	printf("Unable to init parameter structs\n");
	return 1;
    }
    if (heap != heap_shm_ptr || topsection_ptr != tsp) {
	// FIXME
	printf("Unknown failure initializing parameter structs\n");
	printf("  heap=%p, heap_shm_ptr=%p, topsection_ptr=%zu/%p, tsp=%zu/%p\n",
	       heap, heap_shm_ptr,
	       node_ptr_to_off(topsection_ptr), topsection_ptr,
	       node_ptr_to_off(tsp), tsp);
	return 1;
    }

    return 0;
}


// /tmp/machinekit/machinekit/src/rtapi/rtapi_heap_private.h
// /tmp/machinekit/machinekit/src/rtapi/rtapi_heap.h
