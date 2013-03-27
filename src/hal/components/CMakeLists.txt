add_executable (halsampler sampler_usr.c)
target_link_libraries (halsampler linuxcnchal)

add_executable (halstreamer streamer_usr.c)
target_link_libraries (halstreamer linuxcnchal)

set_property (TARGET halsampler halstreamer APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )

