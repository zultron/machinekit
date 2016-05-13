#!/bin/bash -e
rs274 -g test.ngc | awk '{$1=""; print}'
exit ${PIPESTATUS[0]}
