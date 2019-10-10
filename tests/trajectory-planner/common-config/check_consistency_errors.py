import glob
import sys
import os


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


