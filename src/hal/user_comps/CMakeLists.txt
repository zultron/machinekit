add_executable (gs2_vfd gs2_vfd.c modbus.c)
# modbus uses glib-2.0
set_property (TARGET gs2_vfd
  APPEND PROPERTY INCLUDE_DIRECTORIES ${GLIB2_INCLUDE_DIRS})
target_link_libraries (gs2_vfd linuxcnchal glib-2.0)

add_executable (shuttlexpress shuttlexpress.c)
target_link_libraries (shuttlexpress linuxcnchal)


# shared definitions
set_property (TARGET gs2_vfd shuttlexpress APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  )
