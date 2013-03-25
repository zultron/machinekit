# binaries in these directories depend on hal and rtapi includes
include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_SOURCE_DIR}/rtapi)

add_subdirectory (components)
add_subdirectory (user_comps)
