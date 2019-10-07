#ifndef INI_MACROS_HH
#define INI_MACROS_HH
#include "emcglb.h"
#include "debugflags.h"

// Follow interp convention to return on error
#define CHP(call__) \
do { \
    if (call__) { \
        if (emc_debug & EMC_DEBUG_CONFIG) { \
            rcs_print("%s: %d: bad return value from %s\n", __FUNCTION__, __LINE__, #call__); \
        } \
        return -1; \
    } \
} while (0)

#endif // INI_MACROS_HH
