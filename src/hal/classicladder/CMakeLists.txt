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

#cd /tmp/git-jman/linuxcnc-src/src/hal/classicladder && /usr/bin/gcc
#-DULAPI -DSEQUENTIAL_SUPPORT -DHAL_SUPPORT -DDYNAMIC_PLCSIZE
#-DRT_SUPPORT -DOLD_TIMERS_MONOS_SUPPORT -DMODBUS_IO_MASTER
#-I/tmp/git-jman/linuxcnc-src/src/hal/classicladder
#-I/tmp/git-jman/linuxcnc-src/src -I/tmp/git-jman/linuxcnc-src/src/hal
#-I/tmp/git-jman/linuxcnc-src/src/rtapi -I/usr/include/freetype2
#-I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
#-I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include
#-I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0
#-I/usr/include/libgnomeprintui-2.2 -I/usr/include/libgnomeprint-2.2
#-I/usr/include/libgnomecanvas-2.0 -I/usr/include/libart-2.0
#-I/usr/include/libxml2 -I/usr/include/gail-1.0
#-I/usr/include/pixman-1 -I/usr/include/libpng12 -o
#CMakeFiles/classicladder.dir/drawing.c.o -c
#/tmp/git-jman/linuxcnc-src/src/hal/classicladder/drawing.c

#gcc -c -I. -Ilibnml/linklist -Ilibnml/cms -Ilibnml/rcs
#-Ilibnml/inifile -Ilibnml/os_intf -Ilibnml/nml -Ilibnml/buffer
#-Ilibnml/posemath -Irtapi -Ihal -Iemc/nml_intf -Iemc/kinematics
#-Iemc/motion -Iemc/ini -Iemc/rs274ngc -I/include
#-I/usr/include/python2.6 -Os -g -Wall -DULAPI
#-Wdeclaration-after-statement -I/usr/include/gtk-2.0
#-I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0
#-I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0
#-I/usr/lib64/glib-2.0/include -I/usr/include/pixman-1
#-I/usr/include/freetype2 -I/usr/include/libpng12
#-I/usr/include/libgnomeprintui-2.2 -I/usr/include/libgnomeprint-2.2
#-I/usr/include/libgnomecanvas-2.0 -I/usr/include/libart-2.0
#-I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
#-I/usr/include/libxml2 -I/usr/include/pango-1.0
#-I/usr/include/gail-1.0 -I/usr/include/gtk-2.0
#-I/usr/include/freetype2 -I/usr/include/atk-1.0
#-I/usr/lib64/gtk-2.0/include -I/usr/include/cairo
#-I/usr/include/pixman-1 -I/usr/include/libpng12 -DGTK_INTERFACE
#-DGTK2 -DGNOME_PRINT_USE -DSEQUENTIAL_SUPPORT -DHAL_SUPPORT
#-DDYNAMIC_PLCSIZE -DRT_SUPPORT -DOLD_TIMERS_MONOS_SUPPORT
#-DMODBUS_IO_MASTER hal/classicladder/drawing.c -o
#objects/hal/classicladder/drawing.o

add_executable (classicladder
  ${classicladder_srcs}
  ${CMAKE_SOURCE_DIR}/hal/utils/miscgtk.c
  )

set_property (TARGET classicladder APPEND PROPERTY
  COMPILE_DEFINITIONS ULAPI
  GTK_INTERFACE GTK2  # from the original Submakefile; not in pkg-config
  )


# defines related to our hacked-up version of classicladder?
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
    INCLUDE_DIRECTORIES ${GNOMEPRINT_INCLUDE_DIRS}
    )
  set_property (TARGET ${target_name} APPEND PROPERTY
    COMPILE_DEFINITIONS GNOME_PRINT_USE  # from the original Submakefile
    )
endif (GNOMEPRINT_FOUND)


