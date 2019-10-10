#!/usr/bin/env python 

import sys
import os
import glob
import subprocess
import shutil


COMMON_CONFIG='../common-config'
def find_test_cases():
    cases = []
    for g in ('*.ngc','*.nc','*.TAP'):
        cases.extend(glob.glob(g))
    print cases
    test_cases = [os.path.abspath(c) for c in cases]
    return test_cases


def get_test_path():
    out_name = (sys.argv[1] if len(sys.argv) > 1 else None) or './stderr'
    return os.path.dirname(out_name)


def find_errors(filename):
    error_list=[]
    with open(filename, 'r') as res:
        for l in res.readlines():
            if 'error:' in l:
                error_list.append(l)

    return error_list

def setup_test():
    shutil.copyfile(os.path.join(COMMON_CONFIG, 'sim.tbl.template'), 'sim.tmp.tbl')
    shutil.copyfile(os.path.join(COMMON_CONFIG, 'position.blank'), 'position.tmp.txt')

def run_test(test_case):
    test_name = os.path.basename(test_case)
    test_dir = os.path.dirname(test_case)
    stdout_file = os.path.join(test_dir, 'stdout_' + test_name + '.log')

    print >>sys.stderr, "Running test case", test_case
    setup_test()
    with open(stdout_file, 'w') as f_out:
        f_out.write('{{start_collection: "{}"}}\n'.format(test_case))
        p = subprocess.Popen([
            'linuxcnc',
            '-r',
            'data-collection.ini', test_case], stdout=f_out)
        p.wait()
        f_out.write('{{end_collection: "{}"}}\n'.format(test_case))
        return p.returncode


if __name__ == '__main__':
    errors=[]
    for f in glob.glob("stdout_*ngc*"):
        os.remove(f)

    errors = []
    for t in find_test_cases():
        if run_test(t) != 0:
            sys.exit(1)
        if find_errors(os.path.join(get_test_path(), 'stderr')):
            # Forc checkresult to catch this
            sys.exit(0)
