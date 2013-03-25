#include hal (config_module.h)


INCLUDE_DIRECTORIES (
  ${CMAKE_SOURCE_DIR}/rtapi
  ${CMAKE_SOURCE_DIR}/hal
  )

set (UserMode_PCI_Srcs
  device.c
  firmware.c
  string.c
  )

# Source files
add_library (usermode_pci SHARED
  ${UserMode_PCI_Srcs}
  )

# Add -DRTAPI/-DULAPI to target properties
set_property (TARGET usermode_pci
  APPEND PROPERTY COMPILE_DEFINITIONS RTAPI ${RTAPI_DEFINES})

# Add CFLAGS
set_property (TARGET usermode_pci
  APPEND PROPERTY COMPILE_FLAGS ${RTAPI_CFLAGS})



# Unneeded definitions?
# -Drealtime


# gcc -shared -Bsymbolic -L/tmp/git-jman/linuxcnc-src/lib -Wl,-rpath,/tmp/git-jman/linuxcnc-src/lib -o ../rtlib/hostmot2.so objects/hostmot2.tmp -lm