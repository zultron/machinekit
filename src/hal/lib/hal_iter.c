#include "hal_iter.h"

int halpr_foreach_comp(const char *name,
		       hal_comp_callback_t callback, void *cb_data)
{
    hal_comp_t *comp;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_comp called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_comp called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the comp */
    next = hal_data->comp_list_ptr;
    while (next != 0) {
	comp = SHMPTR(next);
	if (!name || (strcmp(comp->name, name)) == 0) {
	    nvisited++;
	    /* this is the right comp */
	    if (callback) {
		result = callback(comp, cb_data);
		if (result < 0) {
		    // callback signaled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signaled 'stop iterating'.
		    // pass back the number of visited comps.
		    return nvisited;
		} else {
		    // callback signaled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count comps
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = comp->next_ptr;
    }
    /* if we get here, we ran through all the comps, so return count */
    return nvisited;
}


int halpr_foreach_sig(const char *name,
		       hal_sig_callback_t callback, void *cb_data)
{
    hal_sig_t *sig;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_sig called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_sig called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the sig */
    next = hal_data->sig_list_ptr;
    while (next != 0) {
	sig = SHMPTR(next);
	if (!name || (strcmp(sig->name, name)) == 0) {
	    nvisited++;
	    /* this is the right sig */
	    if (callback) {
		result = callback(sig, cb_data);
		if (result < 0) {
		    // callback signaled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signaled 'stop iterating'.
		    // pass back the number of visited sigs.
		    return nvisited;
		} else {
		    // callback signaled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count sigs
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = sig->next_ptr;
    }
    /* if we get here, we ran through all the sigs, so return count */
    return nvisited;
}

int halpr_foreach_thread(const char *name,
		       hal_thread_callback_t callback, void *cb_data)
{
    hal_thread_t *thread;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_thread called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_thread called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the thread */
    next = hal_data->thread_list_ptr;
    while (next != 0) {
	thread = SHMPTR(next);
	if (!name || (strcmp(thread->name, name)) == 0) {
	    nvisited++;
	    /* this is the right thread */
	    if (callback) {
		result = callback(thread, cb_data);
		if (result < 0) {
		    // callback threadnaled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signaled 'stop iterating'.
		    // pass back the number of visited threads.
		    return nvisited;
		} else {
		    // callback signaled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count threads
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = thread->next_ptr;
    }
    /* if we get here, we ran through all the threads, so return count */
    return nvisited;
}

int halpr_foreach_funct(const char *name,
		       hal_funct_callback_t callback, void *cb_data)
{
    hal_funct_t *funct;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_funct called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_funct called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the funct */
    next = hal_data->funct_list_ptr;
    while (next != 0) {
	funct = SHMPTR(next);
	if (!name || (strcmp(funct->name, name)) == 0) {
	    nvisited++;
	    /* this is the right funct */
	    if (callback) {
		result = callback(funct, cb_data);
		if (result < 0) {
		    // callback signaled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signaled 'stop iterating'.
		    // pass back the number of visited functs.
		    return nvisited;
		} else {
		    // callback signaled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count functs
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = funct->next_ptr;
    }
    /* if we get here, we ran through all the functs, so return count */
    return nvisited;
}

int halpr_foreach_ring(const char *name,
		       hal_ring_callback_t callback, void *cb_data)
{
    hal_ring_t *ring;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_ring called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_ring called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the ring */
    next = hal_data->ring_list_ptr;
    while (next != 0) {
	ring = SHMPTR(next);
	if (!name || (strcmp(ring->name, name)) == 0) {
	    nvisited++;
	    /* this is the right ring */
	    if (callback) {
		result = callback(ring, cb_data);
		if (result < 0) {
		    // callback signaled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signaled 'stop iterating'.
		    // pass back the number of visited rings.
		    return nvisited;
		} else {
		    // callback signaled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count rings
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = ring->next_ptr;
    }
    /* if we get here, we ran through all the rings, so return count */
    return nvisited;
}

// forgotten

int halpr_foreach_pin(const char *name,
		      hal_pin_callback_t callback, void *cb_data)
{
    hal_pin_t *pin;
    int next;
    int nvisited = 0;
    int result;

    if (hal_data == 0) {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_pin called before init\n",
			rtapi_instance);
	return -EINVAL;
    }
    if (hal_data->lock & HAL_LOCK_CONFIG)  {
	hal_print_msg(RTAPI_MSG_ERR,
			"HAL:%d ERROR: halpr_foreach_pin called while HAL locked\n",
			rtapi_instance);
	return -EPERM;
    }
    /* search for the pin */
    next = hal_data->pin_list_ptr;
    while (next != 0) {
	pin = SHMPTR(next);
	if (!name || (strcmp(pin->name, name)) == 0) {
	    nvisited++;
	    /* this is the right pin */
	    if (callback) {
		result = callback(pin, cb_data);
		if (result < 0) {
		    // callback signalled an error, pass that back up.
		    return result;
		} else if (result > 0) {
		    // callback signalled 'stop iterating'.
		    // pass back the number of visited pins.
		    return nvisited;
		} else {
		    // callback signalled 'OK to continue'
		    // fall through
		}
	    } else {
		// null callback passed in,
		// just count pins
		// nvisited already bumped above.
	    }
	}
	/* no match, try the next one */
	next = pin->next_ptr;
    }
    /* if we get here, we ran through all the pins, so return count */
    return nvisited;
}
