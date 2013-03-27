set (posemath_srcs
  _posemath.c
  posemath.cc
  gomath.c
  sincos.c
  )

include_directories (${CMAKE_SOURCE_DIR}/rtapi)

add_library (posemath SHARED ${posemath_srcs})
set_target_properties (posemath PROPERTIES
  SOVERSION 0)
install (TARGETS posemath DESTINATION lib)
