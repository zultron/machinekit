#!/usr/bin/env python

# KLUDGE bake-in the path to common configs folder
import sys
import linuxcnc
from subprocess import Popen
from testing.linuxcnc_test_support import LinuxcncControl

def run_test(ngc_filename, sample_file):

    e = LinuxcncControl()
    e.log('startup', 'info', 'start')
    e.do_startup(need_home=True)
    e.log('startup', 'info', 'end')

    e.log(ngc_filename, 'info', 'start')
    e.set_mode(linuxcnc.MODE_AUTO)
    e.open_program(ngc_filename)
    e.set_mode(linuxcnc.MODE_AUTO)
    e.run_full_program()
    state_ok = e.wait_on_program(ngc_filename)
    e.log(ngc_filename, 'info', 'end')

    e.flush_errors()

    sys.exit(e.has_error())

if __name__ == '__main__':
    run_test(sys.argv[3], sys.argv[3]+'.samples')
