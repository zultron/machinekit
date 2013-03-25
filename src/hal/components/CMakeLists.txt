add_executable (halsampler sampler_usr.c)
target_link_libraries (halsampler dl linuxcnchal)
set_property (TARGET halsampler APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )

