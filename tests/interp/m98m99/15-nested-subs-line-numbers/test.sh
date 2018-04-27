#!/bin/bash
echo "M98 sub test"
rs274 -g test-m98.ngc -i test.ini | awk '{$1=""; print}'
echo -e "\n\n\n"
echo "RS274ngc sub test"
rs274 -g test-rs274.ngc -i test.ini | awk '{$1=""; print}'
exit ${PIPESTATUS[0]}
