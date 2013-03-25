set (inifile_srcs
    inifile.cc
    )

add_library (linuxcncini ${inifile_srcs})

add_executable (inivar ${inifile_srcs} inivar.cc)
