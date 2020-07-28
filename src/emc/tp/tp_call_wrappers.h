#ifndef TP_CALL_WRAPPERS_H
#define TP_CALL_WRAPPERS_H

#include "tp_enums.h"

// WARNING: assumes that positive or zero return codes are not fatal errors, and will continue execution locally
#define CHP(callee__) \
do { \
    tp_err_t res = (callee__); \
    if (res < TP_ERR_OK) { \
        return res; \
    } \
} while (0)

#endif // TP_CALL_WRAPPERS_H
