#include "interp_testing_util.hh"


int print_interp_error(Interp & interp, int retval, const char* src_file, int line)
{
    if (retval <= INTERP_MIN_ERROR) {
        return retval;
    }

    if (interp.callstack.frame_count()) {
        CallStackFrame const &frame = interp.callstack.get_stack().front();
        fprintf(stderr,"%s:%d: interp_error: %s\n", frame.src_file.c_str(), frame.src_line, interp.error_text(retval).c_str());
    }

    if (emc_debug & EMC_DEBUG_INTERP) {
        for (auto const &frame : interp.callstack.get_stack()) {
            fprintf(stderr,"%s:%d: from: %s\n", frame.src_file.c_str(), frame.src_line, frame.call_name.c_str());
        }
        if (line > 0) {
            fprintf(stderr,"%s:%d: from G-code: %s\n", src_file, line, interp._setup.blocktext);
        } else {
            fprintf(stderr,"%s:%d: no source\n", src_file, line);
        }
        fprintf(stderr,"---\n");
    }
    return retval;
}
