#!/usr/bin/env python
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


if __name__ == '__main__':
    res_files = glob.glob('nc_files/*.stdout')
    out_file = os.path.join(get_test_path(), 'stderr')
    errors = []
    if res_files:
        print("Found result files:")
        for f in res_files:
            errors.extend(find_errors(f))
            print(f)

    errors.extend(find_errors(out_file))

    if errors:
        print("Found errors:")
        for e in errors:
            print(e.rstrip())

        sys.exit(1)
