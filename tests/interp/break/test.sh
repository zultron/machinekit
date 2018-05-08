#!/bin/bash
rs274 -g test.ngc | awk '/MESSAGE/ {$1=""; print}'
exit ${PIPESTATUS[0]}
