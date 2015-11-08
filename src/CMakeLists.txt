######################################################
# Set up project

cmake_minimum_required (VERSION 2.8)
project (LinuxCNC)


######################################################
# Default settings

# Set default C++ compiler (/usr/bin/c++ by default)
set (CMAKE_CXX_COMPILER "/usr/bin/g++")

# Local CMake scripts directory e.g. Xenomai
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cMake)

# Build shared libraries by default
set (BUILD_SHARED_LIBS TRUE)

# Always add -I.
set (CMAKE_INCLUDE_CURRENT_DIR ON)

# Put binaries into ../bin
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)

# Put libraries into ../lib
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../lib)

# -I top level directory for config.h
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

# set default C and C++ flags
set (DEBUG_CFLAGS "-g -Wall")
set (CMAKE_C_FLAGS "-Os ${DEBUG_CFLAGS}")
set (CMAKE_CXX_FLAGS "-Os ${DEBUG_CFLAGS}")


######################################################
# Check dependencies

# pkg-config, which we'll piggy-back off of
find_package (PkgConfig REQUIRED)

# glib2 (at least hal/user_comps/modbus.c)
# FIXME:  This doesn't find the header file, dangit
pkg_check_modules (GLIB2 REQUIRED glib-2.0)

# readline (halcmd)
# FIXME:  This needs to check for readline v.5; v.6 is GPLv3, IIRC
find_package (Readline REQUIRED)


######################################################
# Build config.h

# need a configure_file() here for config.h
# use file(APPEND) if needed


######################################################
# Build subdirectories

add_subdirectory (rtapi)
add_subdirectory (hal)


