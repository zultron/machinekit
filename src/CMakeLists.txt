cmake_minimum_required (VERSION 2.8)
project (LinuxCNC)

# Local CMake scripts directory e.g. Xenomai
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cMake)

# Build shared libraries by default
set (BUILD_SHARED_LIBS TRUE)

# Put binaries into ../bin
set (RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)

# config.h is in the top level directory
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

set (DEBUG_CFLAGS " -g -Wall")
set (COMPILE_FLAGS "-Os ${DEBUG_CFLAGS}")


# subdirectories to build
add_subdirectory (rtapi)
add_subdirectory (libnml)
add_subdirectory (libnml/posemath)
add_subdirectory (libnml/inifile)


# CFLAGS defines for RTAPI; these appear to be needed by classicladder
# from old Makefile
set (CLASSICLADDER_DEFINES
  SEQUENTIAL_SUPPORT DYNAMIC_PLCSIZE RT_SUPPORT OLD_TIMERS_MONOS_SUPPORT
  MODBUS_IO_MASTER HAL_SUPPORT)


# need a configure_file here for config.h

# file(APPEND) can be used to write stuff into files
