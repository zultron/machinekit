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
if (GNOMEPRINT_FOUND)
  set (classicladder_srcs ${classicladder_srcs}
    print_gnome.c)
  # from the original Submakefile
  set (gnomeprint_defs GNOME_PRINT_USE)
endif ()


add_executable (classicladder
  ${classicladder_srcs}
  ${CMAKE_SOURCE_DIR}/hal/utils/miscgtk.c
  )

set_property (TARGET classicladder APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  # from the original Submakefile; not in pkg-config
  GTK_INTERFACE GTK2
  # defines related to our hacked-up version of classicladder?
  COMPILE_DEFINITIONS SEQUENTIAL_SUPPORT HAL_SUPPORT DYNAMIC_PLCSIZE
  RT_SUPPORT OLD_TIMERS_MONOS_SUPPORT MODBUS_IO_MASTER
  # see above
  ${gnomeprint_defs}
  )

set_property (TARGET classicladder APPEND PROPERTY
  INCLUDE_DIRECTORIES
  # from find_package(GTK2)
  ${GTK2_INCLUDE_DIRS}
# from pkg_check_modules(GNOMEPRINT)
  ${GNOMEPRINT_INCLUDE_DIRS}
 )

target_link_libraries (classicladder
  linuxcnchal
  # from find_package(GTK2)
  ${GTK2_LIBRARIES}
  # from pkg_check_modules(GNOMEPRINT)
  ${GNOMEPRINT_LIBRARIES}
  )
