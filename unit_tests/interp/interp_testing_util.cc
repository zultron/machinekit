#include "interp_testing_util.hh"


int print_interp_error(Interp & interp, int retval, const char* src_file, int line)
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
        if (line > 0) {
            fprintf(stderr,"%s:%d: from G-code: %s\n", src_file, line, interp._setup.blocktext);
        }
        fprintf(stderr,"\n");
    }
    return retval;
}

int execute_(Interp & interp, const char * cmd, const char * src_file, int line)
{
    return print_interp_error(interp, interp.execute(cmd), src_file, line);
}

int execute(Interp & interp, const char * cmd)
{
    return execute_(interp, cmd, "", -1);
}
