set (inifile_srcs
    inifile.cc
    )

add_library (linuxcncini ${inifile_srcs})
set_target_properties (linuxcncini PROPERTIES
  SOVERSION 0)
install (TARGETS linuxcncini DESTINATION lib)

# Why not load liblinuxcncini.so?
add_executable (inivar ${inifile_srcs} inivar.cc)
install (TARGETS inivar DESTINATION bin)
