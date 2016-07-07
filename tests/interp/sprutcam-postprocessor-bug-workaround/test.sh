#!/bin/bash -e
for t in test-alpha.ngc test-alphanumeric.ngc test-numericalpha.ngc; do
    echo $t
    rs274 -g $t | awk '{$1=""; print}'
    if test ${PIPESTATUS[0]} != 0; then exit 1; fi
done
