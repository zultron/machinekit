cmd_/home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o := gcc -Wp,-MD,/home/rogge/repos/lcnc_tormach/src/libnml/posemath/._posemath.o.d  -nostdinc -isystem /usr/lib/gcc/i486-linux-gnu/4.4.3/include  -Iinclude  -I/usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include -include include/linux/autoconf.h -Iubuntu/include  -D__KERNEL__ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -m32 -msoft-float -mregparm=3 -freg-struct-return -mpreferred-stack-boundary=2 -march=i586 -mtune=generic -maccumulate-outgoing-args -Wa,-mtune=generic32 -ffreestanding -fstack-protector -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -pipe -Wno-sign-compare -fno-asynchronous-unwind-tables -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -Wframe-larger-than=1024 -fno-omit-frame-pointer -fno-optimize-sibling-calls -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -Os -I. -I/usr/realtime-2.6.32-122-rtai/include -I. -I/usr/realtime-2.6.32-122-rtai/include -D_FORTIFY_SOURCE=0 -mhard-float -DRTAI=3 -DRTAPI -D_GNU_SOURCE -Drealtime -D_FORTIFY_SOURCE=2 -D__MODULE__ -I/home/rogge/repos/lcnc_tormach/src -I/home/rogge/repos/lcnc_tormach/src/libnml/linklist -I/home/rogge/repos/lcnc_tormach/src/libnml/cms -I/home/rogge/repos/lcnc_tormach/src/libnml/rcs -I/home/rogge/repos/lcnc_tormach/src/libnml/inifile -I/home/rogge/repos/lcnc_tormach/src/libnml/os_intf -I/home/rogge/repos/lcnc_tormach/src/libnml/nml -I/home/rogge/repos/lcnc_tormach/src/libnml/buffer -I/home/rogge/repos/lcnc_tormach/src/libnml/posemath -I/home/rogge/repos/lcnc_tormach/src/rtapi -I/home/rogge/repos/lcnc_tormach/src/hal -I/home/rogge/repos/lcnc_tormach/src/emc/nml_intf -I/home/rogge/repos/lcnc_tormach/src/emc/kinematics -I/home/rogge/repos/lcnc_tormach/src/emc/motion -DSEQUENTIAL_SUPPORT -DHAL_SUPPORT -DDYNAMIC_PLCSIZE -DRT_SUPPORT -DOLD_TIMERS_MONOS_SUPPORT -DMODBUS_IO_MASTER -fno-fast-math -mieee-fp -fno-unsafe-math-optimizations -Wframe-larger-than=2560  -DMODULE -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(_posemath)"   -c -o /home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o /home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.c

deps_/home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o := \
  /home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.c \
  /home/rogge/repos/lcnc_tormach/src/libnml/posemath/posemath.h \
  /home/rogge/repos/lcnc_tormach/src/rtapi/rtapi_math.h \
  /usr/lib/gcc/i486-linux-gnu/4.4.3/include/float.h \
  /home/rogge/repos/lcnc_tormach/src/rtapi/rtapi_math_i386.h \
  /home/rogge/repos/lcnc_tormach/src/rtapi/rtapi_byteorder.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/types.h \
    $(wildcard include/config/x86/64.h) \
    $(wildcard include/config/highmem64g.h) \
  include/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/x86/32.h) \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/posix_types_32.h \
  include/linux/swab.h \
  /usr/src/linux-headers-2.6.32-122-rtai/arch/x86/include/asm/swab.h \
    $(wildcard include/config/x86/bswap.h) \
  include/linux/byteorder/generic.h \
  /home/rogge/repos/lcnc_tormach/src/libnml/posemath/sincos.h \
  /home/rogge/repos/lcnc_tormach/src/config.h \
    $(wildcard include/config/h.h) \
    $(wildcard include/config/dir.h) \

/home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o: $(deps_/home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o)

$(deps_/home/rogge/repos/lcnc_tormach/src/libnml/posemath/_posemath.o):
