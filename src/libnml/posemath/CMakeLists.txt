set (posemath_srcs
  _posemath.c
  posemath.cc
  gomath.c
  sincos.c
  )

include_directories (${CMAKE_SOURCE_DIR}/rtapi)

add_library (posemath ${posemath_srcs})
