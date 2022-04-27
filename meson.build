# Non-recursive Meson build for LinuxCNC unit tests
#
# Individual subdirectories define files / include directories for use in the top-level build
# Avoid defining targets / libraries / dependencies in subdirectories if possible.
# It's a big pain to add an external dependency in a dubdirectory, because it
# needs either kludgy relative paths, or a specific execution order of the
# subdir commands (fragile and hard to maintain / inspect).
#
# Keeping with Meson's general philosophy, the preferred way is to define paths
# for sources and includes in the subdirectories, and the rest in the top-level
# file. This way, the entire dependency tree is present in one file.
# Furthermore, a user needing to add a new source file only has to touch the
# subdirectory file, and it's very obvious what needs to change (no build rule
# clutter there).
#
# General pattern:
# 
# meson.build -> project settings, targets, library definitions, etc.
#    
# src/
#    foo/meson.build -> define files / inc for libfoo
#    bar/meson.build -> define files / inc for libbar
#    baz/meson.build -> define files / inc for libbaz
#
# Suffixes used:
#  _srcs: 'files' object containing source files
#  _inc:  'include_directory' object with internal include paths for a given library (not external dependencies)
#  _external_inc: contains miscellaneous include paths for a library
#    (consider this a temporary fix, includes should ideally be handled by
#     dependency objects)
# _dep:  dependency object (encapsulates include / link dependencies for a library)
#
# TODO
# * Provide unit tests for more libraries beyond interp / motion
# * Build linuxcnc libraries with correct build options given a configuration
#

project('linuxcnc-unit-test', ['c','cpp'], default_options : ['c_std=gnu99', 'cpp_std=c++11'])

# TODO make these arguments specific to libraries if we ever replace autotools
add_global_arguments(['-DULAPI','-DUNIT_TEST','-DTP_PEDANTIC_DEBUG','-Wall','-fgnu89-inline'], language : 'c')
add_global_arguments(['-DULAPI','-DUNIT_TEST'], language : 'cpp')

# Common external dependencies 
m_dep = meson.get_compiler('c').find_library('m', required : true)
boost_dep = dependency('boost', modules : ['python3'])
python3_dep = dependency('python3-embed')

# Define source files and include paths

subdir('src')
subdir('src/emc/nml_intf')
subdir('src/emc/interp_shared')
subdir('src/emc/rs274ngc')
subdir('src/emc/sai')
subdir('src/emc/task')
subdir('src/emc/pythonplugin')
subdir('src/emc/tp')
subdir('src/emc/kinematics')
subdir('src/emc/motion')
subdir('src/hal')
subdir('src/emc/ini')
subdir('src/libnml/buffer')
subdir('src/libnml/cms')
subdir('src/libnml/rcs')
subdir('src/libnml/inifile')
subdir('src/libnml/linklist')
subdir('src/libnml/nml')
subdir('src/libnml/os_intf')
subdir('src/libnml/posemath')
subdir('src/rtapi')
subdir('unit_tests')
subdir('unit_tests/tp')
subdir('unit_tests/interp')
subdir('unit_tests/interp_shared')
subdir('unit_tests/emccanon')
subdir('unit_tests/mock')

# Global library dependencies
dl_dep = meson.get_compiler('cpp').find_library('dl', required : true)
m_dep = meson.get_compiler('cpp').find_library('m', required : true)
rpc_dep = dependency('libtirpc', required: false)

# Source path locations
src_root = 'src'
tp_dir = join_paths(src_root, 'emc/tp')

##############################################################################
# INI file library

liblinuxcncini_inc = [
  inifile_inc,
  config_inc
]

liblinuxcncini = shared_library('inifile',
    inifile_srcs,
    include_directories : liblinuxcncini_inc,
    dependencies : [ ]
    )

liblinuxcncini_dep = declare_dependency(include_directories : liblinuxcncini_inc,
    link_with : liblinuxcncini)

##############################################################################
# Python plugin

libpyplugin = shared_library('pyplugin',
    pythonplugin_srcs,
    include_directories : pythonplugin_inc,
    dependencies : [ boost_dep, python3_dep, liblinuxcncini_dep, dl_dep]
    )

libpyplugin_dep = declare_dependency(include_directories : pythonplugin_inc,
    link_with : libpyplugin)

posemath_external_inc = ([
  config_inc,
  rtapi_inc
])

##############################################################################
# Posemath (C & C++)

libposemath = shared_library('posemath',
    posemath_srcs,
    include_directories : [posemath_inc, posemath_external_inc],
    dependencies : [ m_dep ]
    )

libposemath_dep = declare_dependency(include_directories : posemath_inc,
    link_with : libposemath)

libposemath_cpp = shared_library('posemath_cpp',
    posemath_cpp_srcs,
    include_directories : posemath_inc,
    dependencies : [ dl_dep, m_dep, libposemath_dep]
    )

libposemath_cpp_dep = declare_dependency(include_directories : posemath_inc,
    link_with : libposemath_cpp)

##############################################################################
# NML Linked List

nml_common_inc = [
    buffer_inc,
    cms_inc,
    rcs_inc,
    nml_inc,
    os_intf_inc,
]

liblinklist_inc = [
    linklist_inc,
    nml_common_inc,
]

liblinklist = shared_library('linklist',
    linklist_srcs,
    include_directories : liblinklist_inc,
    dependencies : [ libposemath_dep ]
    )

liblinklist_dep = declare_dependency(include_directories : linklist_inc,
    link_with : liblinklist)

##############################################################################
# LibNML

libnml_inc = [
    nml_common_inc,
    config_inc,
    motion_inc,
]

libnml_srcs = [
  buffer_srcs,
  cms_srcs,
  nml_srcs,
  rcs_srcs,
  timer_srcs,
  shm_srcs,
  os_intf_cpp_srcs,
]


libnml = shared_library('nml',
    libnml_srcs,
    include_directories : [
      libnml_inc,
      interp_shared_inc,
      rs274ngc_inc,
      emcpose_inc,
      ],
    dependencies : [ libposemath_dep, liblinklist_dep, liblinuxcncini_dep, rpc_dep]
    )

libnml_dep = declare_dependency(include_directories : nml_common_inc,
    link_with : libnml)

###############################################################################
# emc misc

libemc_global = shared_library('emc_global',
    emc_global_srcs,
    include_directories : [ emc_global_inc, config_inc, motion_inc],
    dependencies : [ libposemath_dep ]
    )

libemc_global_dep = declare_dependency(include_directories : emc_global_inc,
    link_with : libemc_global)

libemcpose = static_library('emcpose',
  emcpose_srcs,
  include_directories : [ emcpose_inc, posemath_inc, rtapi_inc ],
  dependencies : [ libposemath_dep ]
)

libemcpose_dep = declare_dependency(include_directories : emcpose_inc,
    link_with : libemcpose)

libinterp_shared = shared_module('interp_shared',
  interp_shared_srcs,
  dependencies : [
    libposemath_dep,
    libemcpose_dep,
    ],
  include_directories : [interp_shared_inc, motion_inc]
  )

libinterp_shared_dep = declare_dependency(include_directories : interp_shared_inc,
    link_with : libinterp_shared)

##############################################################################
# NML Interp List

libnml_interp_list_inc = [
    nml_common_inc,
    nml_interp_list_inc,
    config_inc,
    motion_inc,
    rs274ngc_inc,
]

# FIXME avoid converting autotools configuration for now, eventually replace with native config
libnml_interp_list = shared_library('nml_interp_list',
    nml_interp_list_srcs,
    include_directories : libnml_interp_list_inc, 
    dependencies : [ libposemath_dep, liblinklist_dep, libnml_dep, libemc_global_dep, libinterp_shared_dep, ]
    )

libnml_interp_list_dep = declare_dependency(include_directories : nml_interp_list_inc,
    link_with : libnml_interp_list)

##############################################################################
# TP sub-set for unit testing only
libtp_unit_test = shared_library('tp_unit_test',
    include_directories : [posemath_inc, posemath_external_inc],
    dependencies : [ m_dep ]
    )

unit_test_inc = include_directories([
  'unit_tests',
  'unit_tests/mock',
])

tp_unit_test_inc = [
  config_inc,
  kinematics_inc,
  motion_inc,
  rtapi_inc,
  hal_inc,
  emcpose_inc,
]

# ULAPI

libulapi = static_library('ulapi',
  ulapi_srcs,
  include_directories : [rtapi_inc, config_inc],
)

libulapi_dep = declare_dependency(include_directories : rtapi_inc,
    link_with : libulapi)

# Define static libraries for easier linking with unit tests

liblinuxcnchal = library('linuxcnchal',
  hal_lib_srcs,
  dependencies : [m_dep, libulapi_dep],
  include_directories : [hal_inc, tp_unit_test_inc])

liblinuxcnchal_dep = declare_dependency(include_directories : hal_inc,
    link_with : liblinuxcnchal)

# Motion Mocks

libmock_motion_global = library('mock_motion_global',
  [mock_motion_global_srcs],
  include_directories : [
    mock_inc,
    kinematics_inc,
    motion_inc,
    tp_inc,
    hal_inc,
  ],
  dependencies : [
    libemcpose_dep,
    libposemath_dep,
    libulapi_dep,
  ]
)

libmock_motion_global_dep = declare_dependency(include_directories : [mock_inc, motion_inc],
    link_with : libmock_motion_global)

libmock_motion_command = library('mock_motion_command',
  [mock_motion_command_srcs],
  include_directories : [
    mock_inc,
    kinematics_inc,
    motion_inc,
    hal_inc,
  ],
  dependencies : [
    libemcpose_dep,
    libposemath_dep,
    libulapi_dep,
  ]
)

libmock_motion_command_dep = declare_dependency(include_directories : [mock_inc, motion_inc],
    link_with : libmock_motion_command)

# TP helper mocks

libmock_standalone_tp = library('mock_standalone_tp',
  [mock_standalone_tp_srcs, tp_srcs, motion_shared_srcs],
  include_directories : [tp_inc, mock_inc, kinematics_inc],
  dependencies : [
    m_dep,
    libposemath_dep,
    libmock_motion_global_dep,
    libmock_motion_command_dep,
    libemcpose_dep,
    libulapi_dep,
    libemc_global_dep,
    liblinuxcnchal_dep,
  ]
)

libmock_standalone_tp_dep = declare_dependency(include_directories : [mock_inc, tp_inc],
    link_with : libmock_standalone_tp)

# KLUDGE has to match the list of files in tp unit test source folder
tp_test_files = [
  'test_blendmath',
  'test_joint_util',
  'test_spherical_arc',
  'test_posemath',
  'test_pm_vector',
  'test_pm_line9',
  'test_pm_circle9',
  'test_tp_create_segments',
  'test_tcqueue',
  'test_tp_priv',
]

foreach n : tp_test_files
  
test(n, executable(n,
  [join_paths('unit_tests/tp', n+'.c'), greatest_helper_srcs],
  dependencies : [
    m_dep,
    libposemath_dep,
    libmock_motion_global_dep,
    libmock_standalone_tp_dep
  ],
  include_directories : [ tp_unit_test_inc, unit_test_inc ],
  ))

endforeach


rs274ngc_external_inc = [
  config_inc,
  emcpose_inc,
  motion_inc,
  pythonplugin_inc,
  nml_inc,
  posemath_inc,
]

librs274ngc = shared_module('rs274ngc',
  rs274ngc_srcs,
  dependencies : [
    boost_dep, 
    python3_dep,
    liblinuxcncini_dep, 
    liblinuxcnchal_dep,
    libpyplugin_dep,
    libposemath_cpp_dep,
    libinterp_shared_dep,
    ],
  include_directories : [tp_unit_test_inc, rs274ngc_inc, rs274ngc_external_inc]
  )

librs274ngc_dep = declare_dependency(include_directories : rs274ngc_inc,
    link_with : librs274ngc)

libsaicanon = shared_module('saicanon',
  saicanon_srcs,
  include_directories : [sai_inc, rs274ngc_external_inc ],
  dependencies : [boost_dep, python3_dep, dl_dep, liblinuxcncini_dep, librs274ngc_dep, libinterp_shared_dep]
  )

libsaicanon_dep = declare_dependency(include_directories : sai_inc,
    link_with : libsaicanon)


libut_emccanon = shared_module('ut_emccanon',
  [emccanon_srcs, mock_emctask_srcs ],
  include_directories : [task_inc, rs274ngc_external_inc, motion_inc, kinematics_inc, rtapi_inc],
  dependencies : [boost_dep, python3_dep, dl_dep, liblinuxcncini_dep, librs274ngc_dep, libinterp_shared_dep, libposemath_cpp_dep, libnml_interp_list_dep, libnml_dep, liblinuxcncini_dep]
  )

libut_emccanon_dep = declare_dependency(include_directories : task_inc,
    link_with : libut_emccanon)

###########################################################################
# Mocks

common_mock_inc = [
  mock_inc,
  config_inc,
  emc_global_inc,
  motion_inc,
]

libmock_emcglb = library('mock_emcglb',
  [mock_emcglb_srcs],
  include_directories : [
    common_mock_inc,
    posemath_inc,
  ]
)

libmock_emcglb_dep = declare_dependency(include_directories : [mock_inc],
    link_with : libmock_emcglb)

libmock_stat_config = library('mock_stat_config',
  [mock_stat_config_srcs],
  include_directories : common_mock_inc,
  dependencies : [libnml_dep, libposemath_dep, librs274ngc_dep, libinterp_shared_dep, libut_emccanon_dep, libnml_interp_list_dep, libmock_emcglb_dep]
)

libmock_stat_config_dep = declare_dependency(include_directories : [mock_inc],
    link_with : libmock_stat_config)

libinterp_testing_util = library('interp_testing_util',
  interp_testing_util_srcs,
  include_directories : [test_interp_inc, nml_intf_inc, motion_inc, config_inc, unit_test_support_inc ],
  dependencies : [librs274ngc_dep, libinterp_shared_dep, libposemath_dep, libnml_dep, python3_dep, libemc_global_dep, libmock_stat_config_dep]
  )

libinterp_testing_util_dep = declare_dependency(include_directories : test_interp_inc,
    link_with : libinterp_testing_util)

test_interp_ex = executable('test_interp',
    test_interp_srcs,
    include_directories : [test_interp_inc, rs274ngc_external_inc, unit_test_support_inc, nml_intf_inc],
    dependencies: [
        dl_dep,
        python3_dep,
        librs274ngc_dep,
        libpyplugin_dep,
        liblinuxcnchal_dep,
        libut_emccanon_dep,
        libinterp_shared_dep,
        libinterp_testing_util_dep,
        libmock_stat_config_dep,
        libnml_dep,
        libnml_interp_list_dep,
        ]
    )

test('test_interp', test_interp_ex)

test_interp_shared_ex = executable('test_interp_shared',
    test_interp_shared_srcs,
    include_directories : [
      test_interp_shared_inc,
      rs274ngc_inc,
      rs274ngc_external_inc,
      unit_test_support_inc,
      ],
    dependencies: [
        dl_dep,
        libinterp_shared_dep,
        ]
    )

test('test_interp_shared', test_interp_shared_ex)

test_emccanon_ex = executable('test_emccanon',
    test_emccanon_srcs,
    include_directories : [
      test_emccanon_inc,
      rs274ngc_external_inc,
      unit_test_support_inc,
      nml_intf_inc,
      rs274ngc_inc,
      ],
    dependencies: [
        dl_dep,
        python3_dep,
        librs274ngc_dep,
        libpyplugin_dep,
        liblinuxcnchal_dep,
        libut_emccanon_dep,
        libposemath_cpp_dep,
        libinterp_shared_dep,
        libmock_stat_config_dep,
        libmock_emcglb_dep,
        libnml_dep,
        libnml_interp_list_dep,
        libinterp_testing_util_dep,
        ]
    )

test('test_emccanon', test_emccanon_ex)

