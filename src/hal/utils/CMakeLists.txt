# Executables
add_executable (halcmd
  halcmd.c halcmd_commands.c halcmd_main.c halcmd_completion.c
  )
target_link_libraries (halcmd
  linuxcncini linuxcnchal
  ${READLINE_LIBRARY}
  )

add_executable (halrmt
  halrmt.c
  )
target_link_libraries (halrmt
  linuxcnchal
  pthread
  )

add_executable (halmeter
  meter.c miscgtk.c
  )
target_link_libraries (halmeter
  linuxcnchal
  ${GTK_LIBS}
  )

add_executable (halscope 
  scope.c scope_horiz.c scope_vert.c scope_trig.c scope_disp.c
  scope_files.c miscgtk.c
  )
target_link_libraries (halscope
  linuxcnchal
  ${GTK_LIBS}
  )

add_executable (pci_write
  pci_write.c upci.c
  )

add_executable (pci_read
  pci_read.c upci.c
  )

# FIXME
#add_executable (comp
#  comp.py

# FIXME
#add_executable (yapps
# yapps.py


# The TCL interface library
add_library (hal
  halcmd.c halcmd_commands.c halsh.c 
  )
target_link_libraries (hal linuxcncini linuxcnchal)
# call it 'hal.so', not 'libhal.so'
set_property (TARGET hal PROPERTY
  PREFIX ""
  )
# put it in with the tcl files
set_property (TARGET hal PROPERTY
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../tcl
  )


# shared properties
set_property (TARGET hal halcmd halrmt APPEND PROPERTY
  INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/libnml/inifile
  )
set_property (TARGET hal halcmd halrmt halmeter halscope APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )

set_property (TARGET halmeter halscope APPEND PROPERTY
  INCLUDE_DIRECTORIES ${GTK_INCLUDE_DIRECTORIES})
set_property (TARGET halmeter halscope APPEND PROPERTY
  COMPILE_DEFINITIONS ${GTK_COMPILE_DEFINITIONS})
