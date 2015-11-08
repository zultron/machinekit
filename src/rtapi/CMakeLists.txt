# ENABLE_EXPORTS property:  related to dlopen()?

# Detect xenomai user flavor: Turn on or off on cmake commandline with
# -DFLAVOR_XENOMAI_USER=off/in
option (FLAVOR_XENOMAI_USER "Build Xenomai user threads" on)
if (FLAVOR_XENOMAI_USER)
  find_package(Xenomai)
  set (xenomai-user-includes ${XENOMAI_INCLUDE_DIRS})
  include_directories (${XENOMAI_INCLUDE_DIRS})
  list (APPEND userland_thread_styles xenomai-user)
  set (xenomai-user_LIBS ${XENOMAI_LIBRARIES})
  set (xenomai-user_CFLAGS ${XENOMAI_CFLAGS_CLEAN})
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

    # link ulapi_main with ulapi.so
    set (extra_lib_srcs "")
    if (XXApi STREQUAL ulapi)
      set (extra_lib_srcs ulapi_main.c)
    endif ()

    # Build module rtapi.so/ulapi.so
    add_library (${target_name} MODULE
      ${rtapi_common_srcs}
      "${Style}.c"
      ${extra_lib_srcs}
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
    set_property (TARGET ${target_name}
      APPEND PROPERTY COMPILE_FLAGS
      " ${${XXApi_Upper}_CFLAGS} ${${Style}_CFLAGS}")

    # Add any flavor-specific includes
    if (DEFINED ${Style}-includes)
      set_property (TARGET ${target_name}
	APPEND PROPERTY INCLUDE_DIRECTORIES "${${Style}-includes}")
    endif(DEFINED ${Style}-includes)

    # Add any flavor-specific libraries
    target_link_libraries (${target_name}
      ${${Style}_LIBS} ${${Style}_LIBS})


    # FIXME:  this is temporary until dlopen() is working
    if (XXApi STREQUAL rtapi)
      add_executable (rtapi_app_${Style}
	${rtapi_common_srcs}
	"${Style}.c"
	sim_rtapi_app.cc
	)
      set_property (TARGET rtapi_app_${Style} APPEND PROPERTY
	COMPILE_DEFINITIONS RTAPI
	)
      set_target_properties (rtapi_app_${Style} PROPERTIES
	INSTALL_RPATH_USE_LINK_PATH TRUE
	)
      target_link_libraries(rtapi_app_${Style} ${${Style}_LIBS} dl)
      set_property (TARGET rtapi_app_${Style}
	APPEND PROPERTY COMPILE_FLAGS
	" ${${XXApi_Upper}_CFLAGS} ${${Style}_CFLAGS}")
    endif ()

  endforeach (XXApi)
endforeach (Style)


# kdetect and rtapiload executables
add_executable (kdetect rtapi_kdetect.c)
target_link_libraries (kdetect rt linuxcnchal)

add_executable (rtapiload rtapi_load.c)
target_link_libraries (rtapiload linuxcnchal)

set_property (TARGET kdetect rtapiload APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI TEST
  )


# binaries below depend on hal includes
include_directories (${CMAKE_SOURCE_DIR}/hal)

#set_property (TARGET rtapi_app APPEND PROPERTY
#  INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/hal
#  )


# FIXME:  commented out until dlopen() working
# rtapi_app
#/usr/bin/g++ -rdynamic -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib -o ../bin/rtapi_app objects/rtrtapi/rtapi_common.o objects/rtrtapi/rtapi_task.o objects/rtrtapi/rtapi_shmem.o objects/rtrtapi/rtapi_time.o objects/rtrtapi/rtapi_msg.o objects/rtrtapi/rtapi_io.o objects/rtrtapi/xenomai-user.o objects/rtrtapi/rtapi_pci.o objects/rtapi/sim_rtapi_app.o  -lnative -L/usr/lib64 -lxenomai -lpthread -lrt  -ludev   -ldl

# add_executable (rtapi_app sim_rtapi_app.cc)
# set_property (TARGET rtapi_app APPEND PROPERTY
#   COMPILE_DEFINITIONS ULAPI
#   )
#set_property (TARGET rtapi_app APPEND PROPERTY
#  INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/hal
#  )
#
# Does this or the next do anything?  How to get -Wl,-rpath,<rpath>
# into the linker command like in the above g++ command?
# set_property (TARGET rtapi_app APPEND PROPERTY
#   INSTALL_RPATH_USE_LINK_PATH TRUE
#   )
# set_target_properties (rtapi_app PROPERTIES
#   INSTALL_RPATH ${CMAKE_CURRENT_SOURCE_DIR})
# # add -ldl and Xeno libs to linker command
# # FIXME: does the following need -lrt?  (included by default with xenomai)
# # FIXME: does the following need -ludev?  (in original)
# target_link_libraries(rtapi_app dl)

# liblinuxcnchal.so
#gcc -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib  -Wl,-soname,liblinuxcnchal.so.0 -shared \
#	    -o ../lib/liblinuxcnchal.so.0 objects/hal/hal_lib.o objects/rtapi/rtapi_compat.o -ldl  -lnative -L/usr/lib64 -lxenomai -lpthread -lrt
#ln -sf liblinuxcnchal.so.0 ../lib/liblinuxcnchal.so

add_library (liblinuxcnchal
  rtapi_compat.c
  ${CMAKE_SOURCE_DIR}/hal/hal_lib.c
  )
set_property (TARGET liblinuxcnchal APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )
target_link_libraries (liblinuxcnchal dl)
set_target_properties (liblinuxcnchal PROPERTIES
  SOVERSION 0)


# Build the usermode pci code
add_subdirectory (userpci)
