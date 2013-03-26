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
add_subdirectory (libnml)
add_subdirectory (libnml/posemath)
add_subdirectory (libnml/inifile)
add_subdirectory (hal)


