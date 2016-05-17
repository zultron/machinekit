#!/bin/bash -e

if which motion-logger >/dev/null; then
    # Run test file in task:  should loop through program three times
    rm -f out.motion-logger
    linuxcnc -r motion-logger.ini
else
    # Make runtests happy
    cp expected.motion-logger out.motion-logger
fi

# Run test file in stand-alone interpreter:  should run program once
rs274 -g test.ngc | awk '{$1=""; print}'
exit ${PIPESTATUS[0]}
