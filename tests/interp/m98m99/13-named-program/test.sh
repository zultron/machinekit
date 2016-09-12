#!/bin/bash -e
for t in test-named.ngc test-numbered.ngc; do
    rs274 -g $t | awk '{$1=""; print}'
    if test ${PIPESTATUS[0]} != 0; then exit 1; fi
done
