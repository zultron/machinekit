# Define a function to create HAL comps.
#
# This file defines a CMake function to build a HAL component.
# To use it, first include this file.
#
#   include( UseHALComp )
#
# Then call hal_add_comp to create a component.
#
#   hal_comp_add_module( <comp_name> )
#

#=============================================================================
# Copyright 2015 John Morris <john@zultron.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# =============================================================================

# TODO:
#
# If this could be packaged as an out-of-tree comp CMake helper, it
# needs to be generalized, esp. ${MACHINEKIT_SOURCE_DIR}

if(MACHINEKIT_SOURCE_DIR AND EXISTS "${MACHINEKIT_SOURCE_DIR}/hal/utils/comp.g")
  # Assume we're in the Machinekit source tree;
  # set up COMP_EXECUTABLE run from python with local PYTHONPATH
  find_package(PythonInterp 2.7 REQUIRED)
  set(COMP_EXECUTABLE
    env PYTHONPATH=${MACHINEKIT_SOURCE_DIR}/../lib/python
    ${PYTHON_EXECUTABLE} ${MACHINEKIT_BINARY_DIR}/hal/utils/comp)
  set(COMP_EXECUTABLE_TARGET comp_executable)  # for dependencies
else()
  find_program(COMP_EXECUTABLE
    NAMES comp
    )
endif()

# hal_comp_add_module _name [ dependency ... ]
function(hal_comp_add_module _name)
  # Wrapper functions may generate .comp in CURRENT_BINARY_DIR
  if(ARGV2)
    set(comp_source "${CMAKE_CURRENT_BINARY_DIR}/${_name}.comp")
  else()
    set(comp_source "${CMAKE_CURRENT_SOURCE_DIR}/${_name}.comp")
  endif()
  set(comp_c "${_name}.c")
  add_custom_command(OUTPUT ${comp_c}
    COMMAND ${COMP_EXECUTABLE}
    ARGS --require-license -o ${comp_c} ${comp_source}
    DEPENDS ${comp_source} ${COMP_EXECUTABLE_TARGET}
    COMMENT "Preprocessing comp ${_name}"
    )
  add_library(${_name} MODULE
    ${comp_c}
    )
  set_target_properties(${_name} PROPERTIES PREFIX "") # Don't prepend 'lib'
  target_include_directories(${_name} PRIVATE
    # FIXME: This may need work once this file becomes an external
    # CMake FindHALComp module
    $<TARGET_PROPERTY:rtapi,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:rtapi_math,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:linuxcnchal,INTERFACE_INCLUDE_DIRECTORIES>
    )
endfunction()

function(hal_conv_comp_add_module _name in_type out_type)
  if(ARGN)
    set(min ${ARGV3})
    set(max ${ARGV4})
  else()
    set(comment "//")
    set(min 0)
    set(max 0)
  endif()
  if(NOT ${in_type} STREQUAL "float" AND NOT ${out_type} STREQUAL "float")
    set(F "nofp")
  endif()
  add_custom_command(OUTPUT "${_name}.comp"
    COMMAND sed < ${CMAKE_CURRENT_SOURCE_DIR}/conv.comp.in > "${_name}.comp"
    -e "s,@IN@,${in_type},g"
    -e "s,@OUT@,${out_type},g"
    -e "s,@CC@,${comment},g"
    -e "s,@MIN@,${min},g"
    -e "s,@MAX@,${max},g"
    -e "s,@FP@,${F},g"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/conv.comp.in
    COMMENT "Converting conf for and preprocessing ${_name}"
    )    
  hal_comp_add_module(${_name} 1)
endfunction()

function(hal_driver_comp_add_module _name)
  hal_comp_add_module(${_name})
  # Add userpci header directory
  target_include_directories(${_name} PRIVATE
    # FIXME: Hard-code header dir until I know how external CMake Find
    # modules work and until the userpci CMake config is done
    ${MACHINEKIT_SOURCE_DIR}/rtapi/userpci/include
    )
endfunction()

