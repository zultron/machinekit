#include "interp_return.hh"

bool isInterpError(InterpReturn err) {
    return err > INTERP_MIN_ERROR;
}
