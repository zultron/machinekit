#!/bin/bash

# Fanuc enabled:
# - with Fanuc sub:  pass
rs274 -g -i test-fanuc.ini test-fanuc.ngc 2>&1
# - with rs274ngc sub:  pass (rs274ngc subs not disabled)
rs274 -g -i test-fanuc.ini test-rs274ngc.ngc 2>&1

# Fanuc disabled:
# - with Fanuc sub:  fail
! rs274 -g -i test-no-fanuc.ini test-fanuc.ngc 2>&1 || exit 1
# - with rs274ngc sub:  pass
rs274 -g -i test-no-fanuc.ini test-rs274ngc.ngc 2>&1

# Default (Fanuc enabled):
# - with Fanuc sub:  pass
rs274 -g test-fanuc.ngc 2>&1
# - with rs274ngc sub:  pass (rs274ngc subs not disabled)
rs274 -g test-rs274ngc.ngc 2>&1
