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

# FIXME: For some reason the original had an rpath set; should this be
# present?
add_library (libnml ${libnml_srcs})

include_directories (${NML_Subdirs})
include_directories (inifile posemath)

set_property (TARGET libnml
  APPEND PROPERTY INCLUDE_DIRECTORIES "${${Style}-includes}")
set_target_properties (libnml PROPERTIES
  SOVERSION 0)
