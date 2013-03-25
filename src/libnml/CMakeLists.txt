# /usr/bin/g++ -c -I. -Ilibnml/linklist -Ilibnml/cms -Ilibnml/rcs -Ilibnml/inifile -Ilibnml/os_intf -Ilibnml/nml -Ilibnml/buffer -Ilibnml/posemath -Irtapi -Ihal -Iemc/nml_intf -Iemc/kinematics -Iemc/motion -Iemc/ini -Iemc/rs274ngc -I/include -I/usr/include/python2.6 -Os -g -Wall -DULAPI  -fPIC libnml/inifile/inifile.cc -o objects/libnml/inifile/inifile.o

# /usr/bin/g++ -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib -Wl,-soname,liblinuxcncini.so.0 -shared -o ../lib/liblinuxcncini.so.0 objects/libnml/inifile/inifile.o

# ln -sf liblinuxcncini.so.0 ../lib/liblinuxcncini.so


# Linking inivar
# /usr/bin/g++ -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib -o ../bin/inivar objects/libnml/inifile/inivar.o ../lib/liblinuxcncini.so.0 

# gcc -c -I. -Ilibnml/linklist -Ilibnml/cms -Ilibnml/rcs -Ilibnml/inifile -Ilibnml/os_intf -Ilibnml/nml -Ilibnml/buffer -Ilibnml/posemath -Irtapi -Ihal -Iemc/nml_intf -Iemc/kinematics -Iemc/motion -Iemc/ini -Iemc/rs274ngc -I/include -I/usr/include/python2.6 -Os -g -Wall -DULAPI  -Wdeclaration-after-statement  -fPIC libnml/posemath/_posemath.c -o objects/libnml/posemath/_posemath.o

# /usr/bin/g++ -c -I. -Ilibnml/linklist -Ilibnml/cms -Ilibnml/rcs -Ilibnml/inifile -Ilibnml/os_intf -Ilibnml/nml -Ilibnml/buffer -Ilibnml/posemath -Irtapi -Ihal -Iemc/nml_intf -Iemc/kinematics -Iemc/motion -Iemc/ini -Iemc/rs274ngc -I/include -I/usr/include/python2.6 -Os -g -Wall -DULAPI  -fPIC libnml/posemath/posemath.cc -o objects/libnml/posemath/posemath.o

set (libnml_srcs
  rcs/rcs_print.cc
  rcs/rcs_exit.cc
  os_intf/_sem.c
  os_intf/_shm.c
  os_intf/_timer.c
  os_intf/sem.cc
  os_intf/shm.cc
  os_intf/timer.cc
  buffer/locmem.cc
  buffer/memsem.cc
  buffer/phantom.cc
  buffer/physmem.cc
  buffer/recvn.c
  buffer/sendn.c
  buffer/shmem.cc
  buffer/tcpmem.cc
  cms/cms.cc
  cms/cms_aup.cc
  cms/cms_cfg.cc
  cms/cms_in.cc
  cms/cms_dup.cc
  cms/cms_pm.cc
  cms/cms_srv.cc
  cms/cms_up.cc
  cms/cms_xup.cc
  cms/cmsdiag.cc
  cms/tcp_opts.cc
  cms/tcp_srv.cc
  nml/cmd_msg.cc
  nml/nml_mod.cc
  nml/nml_oi.cc
  nml/nml_srv.cc
  nml/nml.cc
  nml/nmldiag.cc
  nml/nmlmsg.cc
  nml/stat_msg.cc
  linklist/linklist.cc
)

set (NML_Subdirs
  buffer 
  cms 
  linklist 
  nml 
  os_intf 
  rcs
  )

add_library (libnml ${libnml_srcs})

include_directories (${NML_Subdirs})
include_directories (inifile posemath)

set_property (TARGET libnml
  APPEND PROPERTY INCLUDE_DIRECTORIES "${${Style}-includes}")

# set (NML_Subdirs
#   buffer 
#   cms 
#   linklist 
#   nml 
#   os_intf 
#   rcs
#   )

# include_directories (${CMAKE_CURRENT_SOURCE_DIR})
# add_subdirectory (rcs)
# add_subdirectory (os_intf)
# add_subdirectory (buffer)
# add_subdirectory (cms)
# add_subdirectory (nml)
# add_subdirectory (linklist)
# not in Submakefile?
#  inifile
#  posemath


# add_library (libnml
#   ${NML_RCS_Srcs}
#   ${NML_OS_Intf_Srcs}
#   ${NML_Buffer_Srcs}
#   ${NML_CMS_Srcs}
#   ${NML_Linklist_Srcs}
#   )
# add_library (libnml)

# target_link_libraries (libnml ${NML_Subdirs})

  