#!/usr/bin/env bash
# Start with a clean environment to avoid reusing libinterp_shared
# Assumes path to root linuxcnc directory

# Print out the current environment (in case of build issues)
env

set -x

# Make sure unit tests are built (uses Meson build system)
mkdir -p ../../build
cd ../..
meson ./build

# Run the unit tests and redirect the output to stderr for post-processing by runtests
cd build
meson test -v 1>&2
