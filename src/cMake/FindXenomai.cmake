###############################################################################
#
# CMake script for finding XENOMAI.
#
# Use -DXENOMAI_XENO_CONFIG=<path> to help find the xeno-config executable.
#
# This script creates the following variables:
#  XENOMAI_FOUND: Boolean that indicates if the package was found
#  XENOMAI_CFLAGS_CLEAN:  Non-"-I" CFLAGS
#  XENOMAI_INCLUDE_DIRS: Paths to the necessary header files
#  XENOMAI_CFLAGS:  Both XENOMAI_INCLUDE_DIRS and XENOMAI_CFLAGS_CLEAN
#  XENOMAI_LIBRARIES: Package library link flags, like -lxenomai
#  XENOMAI_LDFLAGS: All linker flags
#  XENOMAI_XENO_CONFIG:  Path to xeno-config
#
###############################################################################

# Locate xeno-config if not defined on the command-line
if (NOT DEFINED XENOMAI_XENO_CONFIG)
  find_program (XENOMAI_XENO_CONFIG xeno-config
    HINTS ENV XENOMAI_XENO_CONFIG)
endif()

if (XENOMAI_XENO_CONFIG STREQUAL XENOMAI_XENO_CONFIG-NOTFOUND)
  message ("Xenomai not found  (set XENOMAI_ROOT_DIR to xeno-config's path)")
  set (XENOMAI_FOUND FALSE)
  set (XENOMAI_NOTFOUND TRUE)

  if (Xenomai_FIND_REQUIRED)
      message(FATAL_ERROR "Xenomai not found")
  endif()
else()
  set (XENOMAI_FOUND TRUE)

  # Use native skin by default
  if(NOT DEFINED XENOMAI_SKIN)
    set (XENOMAI_SKIN native)
  endif()

  # All cflags
  execute_process (
    COMMAND echo -n
	"\$(${XENOMAI_XENO_CONFIG} --skin=${XENOMAI_SKIN} --cflags)"
    OUTPUT_VARIABLE XENOMAI_CFLAGS
    )
  #message("XENOMAI_CFLAGS ${XENOMAI_CFLAGS}")

  # Include directories
  execute_process (
    COMMAND bash -c "for i in ${XENOMAI_CFLAGS}
	do test \${i#-I} = \$i || echo -n \${i#-I}
	done"
    OUTPUT_VARIABLE XENOMAI_INCLUDE_DIRS
    )
  #message("XENOMAI_INCLUDE_DIRS ${XENOMAI_INCLUDE_DIRS}")

  # Clean (non-"-I") cflags
  execute_process (
    COMMAND bash -c "for i in ${XENOMAI_CFLAGS}
	do test \${i#-I} = \$i && echo -n \$i
	done"
    OUTPUT_VARIABLE XENOMAI_CFLAGS_CLEAN
    )
  #message("XENOMAI_CFLAGS_CLEAN ${XENOMAI_CFLAGS_CLEAN}")

  # All ldflags
  execute_process (
    COMMAND echo -n
	"\$(${XENOMAI_XENO_CONFIG} --skin=${XENOMAI_SKIN} --ldflags)"
    OUTPUT_VARIABLE XENOMAI_LDFLAGS
    )
  #message ("XENOMAI_LDFLAGS ${XENOMAI_LDFLAGS}")

  # Library linker flags like -lxenomai; return as a list (with ';'
  # chars)
  execute_process (
    COMMAND bash -c "for i in ${XENOMAI_LDFLAGS}
	do test \${i#-l} = \$i || echo -n \${i#-l}';'
	done"
    OUTPUT_VARIABLE XENOMAI_LIBRARIES
    )
  #message("XENOMAI_LIBRARIES ${XENOMAI_LIBRARIES}")

endif()
