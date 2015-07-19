add_library(linuxcnc STATIC
	src/emcglb.c
	src/emc.cc
	src/emcpose.c
	src/emcargs.cc
	src/emcops.cc
	src/canon_position.cc
	src/interpl.cc
)
target_include_directories(linuxcnc PUBLIC
	include
	$<TARGET_PROPERTY:posemath,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:nml,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:rs274ngc,INTERFACE_INCLUDE_DIRECTORIES>
	$<TARGET_PROPERTY:tp,INTERFACE_INCLUDE_DIRECTORIES>
	)
target_link_libraries(linuxcnc posemath nml rs274ngc)