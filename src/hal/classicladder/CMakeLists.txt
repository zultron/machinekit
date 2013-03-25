set (classicladder_srcs
  arithm_eval.c
  arrays.c
  calc.c
  calc_sequential.c
  classicladder.c
  classicladder_gtk.c
  config.c
  config_gtk.c
  drawing.c
  drawing_sequential.c
  edit.c
  edit_gtk.c
  edit_sequential.c
  editproperties_gtk.c
  emc_mods.c
  files.c
  files_project.c
  files_sequential.c
  manager.c
  manager_gtk.c
  protocol_modbus_master.c
  protocol_modbus_slave.c
  serial_linux.c
  socket_modbus_master.c
  socket_server.c
  spy_vars_gtk.c
  symbols.c
  symbols_gtk.c
  vars_names.c
  vars_access.c
)

set (GTK_CFLAGS GTK_INTERFACE GTK2)

add_executable (classicladder
  ${classicladder_srcs}
  ${CMAKE_SOURCE_DIR}/hal/utils/miscgtk.c
  )

# related to our hacked-up version of classicladder?
set_property (TARGET classicladder APPEND PROPERTY
  COMPILE_DEFINITIONS SEQUENTIAL_SUPPORT HAL_SUPPORT DYNAMIC_PLCSIZE
  RT_SUPPORT OLD_TIMERS_MONOS_SUPPORT MODBUS_IO_MASTER
  )

# from find_package(GTK2)
set_property (TARGET classicladder APPEND PROPERTY
  INCLUDE_DIRECTORIES ${GTK2_INCLUDE_DIRS})
target_link_libraries (classicladder ${GTK2_LIBRARIES})

# from pkg_check_modules(GNOMEPRINT)
if (GNOMEPRINT_FOUND)
  target_link_libraries (classicladder ${GNOMEPRINT_LIBRARIES})
  set_property (TARGET classicladder APPEND PROPERTY
    COMPILE_FLAGS ${GNOMEPRINT_CFLAGS}
    )
  set_property (TARGET ${target_name} APPEND PROPERTY
    COMPILE_DEFINITIONS GNOME_PRINT_USE)
endif (GNOMEPRINT_FOUND)


