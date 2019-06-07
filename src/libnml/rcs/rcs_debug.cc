#include "rcs_print.hh"
#include <unistd.h>		/* getpid() */
#include <_timer.h>

int rcs_print_debug(long flag_to_check, const char *_fmt, ...)
{
    int retval = 0;
    int pid = 0;
    va_list args;
    va_start(args, _fmt);

    if (flag_to_check & rcs_print_mode_flags) {
    pid = getpid();
    rcs_print("(time=%f,pid=%d): ", etime(), pid);
    retval = rcs_vprint(_fmt, args, 0);
    }
    va_end(args);
    return (retval);
}
