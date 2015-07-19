add_library(linuxcncini SHARED
	src/iniaxis.cc
	src/initool.cc
	src/initraj.cc
	src/inihal.cc
	src/emcIniFile.cc
)
target_include_directories(linuxcncini PUBLIC
	include
	$<TARGET_PROPERTY:linuxcnc,INTERFACE_INCLUDE_DIRECTORIES>
	)
target_link_libraries(linuxcncini linuxcnc)