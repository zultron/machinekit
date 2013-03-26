cmake_minimum_required (VERSION 2.8)
project (LinuxCNC)

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

# Add ../lib to linker search path
# (not needed, since our lib output dir is here)
#link_directories (${CMAKE_SOURCE_DIR}/../lib)

# config.h is in the top level directory
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

set (DEBUG_CFLAGS " -g -Wall")
set (COMPILE_FLAGS "-Os ${DEBUG_CFLAGS}")


# find libraries

# pkg-config, which we'll piggy-back off of
find_package (PkgConfig REQUIRED)

# glib2 (at least hal/user_comps/modbus.c)
# FIXME:  This doesn't find the header file, dangit
pkg_check_modules (GLIB2 REQUIRED glib-2.0)

# gtk+-2.0 (everything graphical)
find_package (GTK2 REQUIRED)
# ...and libgnomeprintui-2.2
if (GTK2_FOUND)
  pkg_check_modules (GNOMEPRINT libgnomeprintui-2.2)
endif (GTK2_FOUND)
# this is needed a LOT, so just set variables right here
set (GTK_INCLUDE_DIRECTORIES ${GTK2_INCLUDE_DIRS})
set (GTK_COMPILE_DEFINITIONS
  GTK_INTERFACE GTK2  # in Submakefile but not pkg-config; origin???
  )
set (GTK_LIBS ${GTK2_LIBRARIES})
if (GNOMEPRINT_FOUND)
  set (GTK_INCLUDE_DIRECTORIES ${GTK_INCLUDE_DIRECTORIES}
    ${GNOMEPRINT_INCLUDE_DIRS}
    )
  set (GTK_COMPILE_DEFINITIONS ${GTK_COMPILE_DEFINITIONS}
    GNOME_PRINT_USE  # in Submakefile but not pkg-config; origin???
    )
  set (GTK_LIBS ${GTK_LIBS}
    ${GNOMEPRINT_LIBRARIES}
    )
endif (GNOMEPRINT_FOUND)

# readline
# FIXME:  This needs to check for readline v.5; v.6 is GPLv3, IIRC
find_package (Readline REQUIRED)

# subdirectories to build
add_subdirectory (rtapi)
add_subdirectory (libnml)
add_subdirectory (libnml/posemath)
add_subdirectory (libnml/inifile)
add_subdirectory (hal)


# CFLAGS defines for RTAPI; these appear to be needed by classicladder
# from old Makefile
set (CLASSICLADDER_DEFINES
  SEQUENTIAL_SUPPORT DYNAMIC_PLCSIZE RT_SUPPORT OLD_TIMERS_MONOS_SUPPORT
  MODBUS_IO_MASTER HAL_SUPPORT)


# need a configure_file here for config.h

# file(APPEND) can be used to write stuff into files
