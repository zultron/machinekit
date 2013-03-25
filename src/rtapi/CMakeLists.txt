# Detect xenomai user flavor: Turn on or off on cmake commandline with
# -DFLAVOR_XENOMAI_USER=off/in
option (FLAVOR_XENOMAI_USER "Build Xenomai user threads" on)
if (FLAVOR_XENOMAI_USER)
  find_package(Xenomai)
  set (xenomai-user-includes ${XENOMAI_INCLUDE_DIRS})
  include_directories (${XENOMAI_INCLUDE_DIRS})
  list (APPEND userland_thread_styles xenomai-user)
endif (FLAVOR_XENOMAI_USER)


# Sources that must be always be built for all thread flavors and for
# rtapi+ulapi
set (rtapi_common_srcs
  rtapi_common.c
  rtapi_task.c
  rtapi_shmem.c
  rtapi_time.c
  rtapi_msg.c
  rtapi_io.c
  )

# Include .h files in .
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

# CFLAGS for RTAPI copied from old Makefile
set (RTAPI_CFLAGS "-fno-fast-math  -fno-unsafe-math-optimizations")
set (RTAPI_CFLAGS "${RTAPI_CFLAGS} -mieee-fp -Wframe-larger-than=2560")

# CFLAGS for ULAPI copied from old Makefile
set (ULAPI_CFLAGS "-Wdeclaration-after-statement")

# Set up to build once each for rtapi and ulapi and for userland styles
foreach (Style ${userland_thread_styles})
  foreach (XXApi rtapi ulapi)
    # Uppercase version:  RTAPI or ULAPI
    string (TOUPPER ${XXApi} XXApi_Upper)

    # Compute target name
    set (target_name "${XXApi}-${Style}")

    # Build shared library
    add_library (${target_name}
      ${rtapi_common_srcs}
      "${Style}.c"
      )

    # Add -DRTAPI/-DULAPI to target properties
    set_property (TARGET ${target_name}
      APPEND PROPERTY COMPILE_DEFINITIONS "${XXApi_Upper}")

    # Add RTAPI/ULAPI -Ddefines, if any
    if (DEFINED ${XXApi_Upper}_DEFINES)
      set_property (TARGET ${target_name}
	APPEND PROPERTY COMPILE_DEFINITIONS ${${XXApi_Upper}_DEFINES})
    endif()

    # Add RTAPI/ULAPI CFLAGS, if any
    if (DEFINED ${XXApi_Upper}_CFLAGS)
      set_property (TARGET ${target_name}
	APPEND PROPERTY COMPILE_FLAGS ${${XXApi_Upper}_CFLAGS})
    endif()

    # Add any flavor-specific includes
    if (DEFINED ${Style}-includes)
      set_property (TARGET ${target_name}
	APPEND PROPERTY INCLUDE_DIRECTORIES "${${Style}-includes}")
    endif(DEFINED ${Style}-includes)

  endforeach (XXApi)
endforeach (Style)

# rtapi_app
#/usr/bin/g++ -rdynamic -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib -o ../bin/rtapi_app objects/rtrtapi/rtapi_common.o objects/rtrtapi/rtapi_task.o objects/rtrtapi/rtapi_shmem.o objects/rtrtapi/rtapi_time.o objects/rtrtapi/rtapi_msg.o objects/rtrtapi/rtapi_io.o objects/rtrtapi/xenomai-user.o objects/rtrtapi/rtapi_pci.o objects/rtapi/sim_rtapi_app.o  -lnative -L/usr/lib64 -lxenomai -lpthread -lrt  -ludev   -ldl

add_executable (rtapi_app sim_rtapi_app.cc)
set_property (TARGET rtapi_app APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )
set_property (TARGET rtapi_app APPEND PROPERTY
  INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/hal
  )
set_property (TARGET rtapi_app APPEND PROPERTY
  INSTALL_RPATH_USE_LINK_PATH TRUE
  )
set_target_properties (rtapi_app PROPERTIES
  INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR})
# does the following need -lrt?
target_link_libraries(rtapi_app dl ${XENOMAI_LIBRARIES})

# Build the usermode pci code
ADD_SUBDIRECTORY (userpci)


