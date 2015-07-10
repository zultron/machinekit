cmake_minimum_required(VERSION 2.8)
project(machinekit)

# Generate config.h
configure_file(config.h.in config.h)
include_directories("${PROJECT_BINARY_DIR}")
# TODO: Add flavors and _GNU_SOURCE to config.h
# TODO: Do not globally include directories, use sub projects and target_include_directories

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(ULAPI 1)
set(EMC2_SYSTEM_CONFIG_DIR "${SYSCONFDIR}")

set(MACHINEKIT_COMPILE_DEFINITIONS "ULAPI=${ULAPI};EMC2_SYSTEM_CONFIG_DIR=\"${EMC2_SYSTEM_CONFIG_DIR}\"")

add_subdirectory(inifile)
add_subdirectory(linklist)
add_subdirectory(machinetalk)
add_subdirectory(rtapi)
add_subdirectory(hal)

