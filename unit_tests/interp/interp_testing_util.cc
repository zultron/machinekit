#include "interp_testing_util.hh"


int print_interp_error(Interp & interp, int retval)
{
    int index = 0;
    if (retval == 0) {
        return retval;
    }

    char interp_error_text_buf[4096] = {};
    interp.error_text(retval, interp_error_text_buf, 4096);
    if (0 != interp_error_text_buf[0]) {
        fprintf(stderr,"%s:%d: interp_error: %s\n", interp._setup.stack_file[0], interp._setup.stack_line[0], interp_error_text_buf);
    }

    if (emc_debug & EMC_DEBUG_INTERP) {
        for (index = 1; index < interp._setup.stack_index; ++index) {
            char interp_stack_buf[4096] = {};
            interp.stack_name(index, interp_stack_buf, 4096);
            if (0 == interp_stack_buf[0]) {
                break;
            }
            fprintf(stderr,"%s:%d: from: %s\n", interp._setup.stack_file[index], interp._setup.stack_line[index], interp_stack_buf);
        }
        fprintf(stderr,"\n");
    }
    return retval;
}

int execute(Interp & interp, const char * cmd)
{
    return print_interp_error(interp, interp.execute(cmd));
}
