add_library(pyplugin SHARED
	src/python_plugin.cc)
target_include_directories(pyplugin PUBLIC
	include
	$<TARGET_PROPERTY:inifile,INTERFACE_INCLUDE_DIRECTORIES>
	${Boost_INCLUDE_DIRS}
	${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(pyplugin inifile ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})