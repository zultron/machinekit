find_package (Threads)

add_library(rtapi STATIC
	# TODO: Figure out how to build all flavors and rest of these files.
	# src/rtapi_kdetect.c
	# src/rt-preempt.c
	# src/flavor.c
	# src/rtapi_main.c
	# src/rtapi_app.cc
	# src/rtapi_time.c
	# src/ulapi_main.c
	# src/rtapi_msgd.cc
	# src/rtapi_hexdump.c
	# src/rtapi_exception.c
	# src/ulapi_autoload.c
	# src/test_rtapi_vsnprintf.c
	# src/rtapi_pci.c
	# src/rtai-kernel.c
	# src/xenomai.c
	# src/rtapi_compat.c
	# src/rtapi_support.c
	# src/userpci/firmware.c
	# src/userpci/device.c
	# src/userpci/string.c
	# src/rtapi_shmem.c
	# src/xenomai-kernel.c
	# src/rtapi_task.c
	# src/rtapi_heap.c
	# src/shmdrv/shmdrv.c
	# src/shmdrv/shmdrvapi.c
	# src/shmdrv/mutexwatch.c
	# src/rtapi_module.c
	# src/rtapi_common.c
	src/rtapi_app.cc
	src/rtapi_compat.c
	src/rtapi_hexdump.c
	src/rtapi_support.c
)

target_include_directories(rtapi
PUBLIC
	include
	$<TARGET_PROPERTY:inifile,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:mtalk,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:mtalk_proto,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:linuxcnchal,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:linuxcncshm,INTERFACE_INCLUDE_DIRECTORIES>)

option(USE_REALTIME "Use realtime" OFF)
# TODO: Fix USE_REALTIME

set_target_properties(rtapi PROPERTIES COMPILE_DEFINITIONS "${MACHINEKIT_COMPILE_DEFINITIONS}")
target_link_libraries(rtapi inifile rtapi_math mtalk_proto)


add_executable(flavor
	src/flavor.c
	src/rtapi_compat.c)
set_target_properties(flavor PROPERTIES COMPILE_DEFINITIONS "${MACHINEKIT_COMPILE_DEFINITIONS}")
target_include_directories(flavor PUBLIC
	include
	$<TARGET_PROPERTY:inifile,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(flavor inifile)