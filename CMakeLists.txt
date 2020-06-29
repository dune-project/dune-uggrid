# set up project
project("dune-uggrid" C CXX)

#circumvent not building docs
set(BUILD_DOCS 1)

# general stuff
cmake_minimum_required(VERSION 3.13)

# Guess the dune-common build directory if it is not yet set
if(NOT (dune-common_DIR OR dune-common_ROOT OR
      "${CMAKE_PREFIX_PATH}" MATCHES ".*dune-common.*"))
  string(REPLACE  ${CMAKE_PROJECT_NAME} dune-common dune-common_DIR
    ${PROJECT_BINARY_DIR})
endif()

#find dune-common and set the module path
find_package(dune-common REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH}
     "${CMAKE_SOURCE_DIR}/cmake/modules")# make sure our own modules are found

#include the dune macros
include(DuneMacros)

# start a dune project with information from dune.module
dune_project()

find_package(MPI)
# set defines that are only used internally.
# The rest is in cmake/modules/DuneUggrid.cmake
if(MPI_C_FOUND)
  set(UG_ENABLE_PARALLEL True CACHE BOOL "Enable a parallel UG (default is True if MPI is there False otherwise)")
else()
  set(UG_ENABLE_PARALLEL False CACHE BOOL "Enable a parallel UG (default is True if MPI is there False otherwise)")
endif()

set(UG_ENABLE_DEBUGGING False CACHE BOOL "Enable UG debugging (default is Off)")
set(UG_ENABLE_2D True CACHE BOOL "Build library for 2d (default is On)")
set(UG_ENABLE_3D True CACHE BOOL "Build library for 3d (default is On)")
set(UG_ENABLE_SYSTEM_HEAP ON CACHE BOOL "If ON/True then we are using the operating system heap instead of the one internal to UG (Default: ON)")
set(UG_DDD_MAX_MACROBITS "24" CACHE STRING
  "Set number of bits of an unsigned int used to store the process number,
       the remaining bits are used to store the local entity id")

set(DUNE_UGGRID_TET_RULESET True CACHE BOOL "Use complete rule set for refinement of tetrahedral elements")
if(TET_RULESET)
  set(DUNE_UGGRID_TET_RULESET True)
  message(DEPRECATION "The TET_RULESET option has been renamed to DUNE_UGGRID_TET_RULESET")
endif()

if(UG_ENABLE_DEBUGGING)
  add_definitions("-DDebug")
  set(UG_EXTRAFLAGS "${UG_EXTRAFLAGS} -DDebug")
endif()

#Always build parallel libs if MPI is found
if(UG_ENABLE_PARALLEL)
  if(NOT MPI_C_FOUND)
    message(SEND_ERROR "A parallel UG was requested but MPI was not found. Either change variable UG_ENABLE_PARALLEL or install MPI." )
  endif()
  add_definitions("-DModelP")
  set(UG_EXTRAFLAGS "${UG_EXTRAFLAGS} -DModelP")
endif()

set(UG_ENABLED_DIMENSIONS)
if(UG_ENABLE_2D)
  list(APPEND UG_ENABLED_DIMENSIONS "2")
endif()

if(UG_ENABLE_3D)
  list(APPEND UG_ENABLED_DIMENSIONS "3")
endif()

macro(ug_expand_object_libs var dim)
  set(${var})
  foreach(_lib ${ARGN})
    set(${var} ${${var}} "\$<TARGET_OBJECTS:${_lib}${dim}>")
  endforeach()
endmacro()

# a macro that creates the libraries for all dimensions.
#
macro(ug_add_dim_libs LIB_NAME)
  cmake_parse_arguments(ADD_LIB  "OBJECT;DUNE" ""
    "SOURCES;SOURCES_2D;SOURCES_3D;OBJECT_LIBS;OBJECT_DIM_LIBS;ADD_LIBS" ${ARGN})
  if(ADD_LIB_APPEND)
    set(_append "APPEND") # append all libraries in export.
  endif()
  if(ADD_LIB_OBJECT)
    set(OBJECT "OBJECT")
  endif()

  ug_expand_object_libs(OBJECT_LIBS "" ${ADD_LIB_OBJECT_LIBS})
  foreach(dim  ${UG_ENABLED_DIMENSIONS})
    ug_expand_object_libs(OBJECT_DIM_LIBS ${dim} ${ADD_LIB_OBJECT_DIM_LIBS})

    if(ADD_LIB_DUNE)
      if(ADD_LIB_ADD_LIBS)
        set(_EXTRA_ARGS "ADD_LIBS" ${ADD_LIB_ADD_LIBS})
      endif()
      if(OBJECT)
        message(FATAL_ERROR "OBJECT may only be used in conjuction with DUNE property")
      endif()
      dune_add_library("${LIB_NAME}${dim}" ${OBJECT} ${ADD_LIB_SOURCES}
        ${ADD_LIB_SOURCES_${dim}D}
        ${ADD_LIB_UNPARSED_ARGUMENTS} ${OBJECT_DIM_LIBS} ${OBJECT_LIBS}
        ${_EXTRA_ARGS} ${_append})
      set(_libs "${LIB_NAME}${dim}")
      if(DUNE_BUILD_BOTH_LIBS)
        set(_libs ${_libs} "${LIB_NAME}${dim}-shared")
      endif()
    else()
      add_library("${LIB_NAME}${dim}" ${OBJECT} ${ADD_LIB_SOURCES}
        ${ADD_LIB_SOURCES_${dim}D}
        ${ADD_LIB_UNPARSED_ARGUMENTS} ${OBJECT_DIM_LIBS} ${OBJECT_LIBS})
      if(OBJECT AND DUNE_BUILD_BOTH_LIBS)
        set_property(TARGET "${LIB_NAME}${dim}" PROPERTY POSITION_INDEPENDENT_CODE TRUE)
      endif()
      dune_target_link_libraries(${LIB_NAME}${dim} ${PROJECT_NAME})
      if(ADD_LIB_ADD_LIBS)
        dune_target_link_libraries(${LIB_NAME}${dim} ${ADD_LIB_ADD_LIBS})
      endif()
      set(_libs "${LIB_NAME}${dim}")
    endif()
    foreach(_lib ${_libs})
      target_compile_definitions("${_lib}" PRIVATE "-DUG_DIM_${dim}")
      if(OBJECT)
        add_dune_mpi_flags("${_lib}" OBJECT)
      else()
        add_dune_mpi_flags("${_lib}")
      endif()
    endforeach()
    set(_append "APPEND") # Do not overwrite but append next lib in export
  endforeach()
endmacro()

check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(time.h HAVE_TIME_H)

if(HAVE_TIME_H AND HAVE_SYS_TIME_H)
  set(TIME_WITH_SYS_TIME True)
endif()

add_subdirectory(dune)
add_subdirectory(cmake/modules)
# set variable names for config.h
set(DDD_MAX_PROCBITS_IN_GID ${UG_DDD_MACROBITS})
# finalize the dune project, e.g., generate config.h etc.
# Use package init to set additional information
set(dune-uggrid_INIT "set(UG_PARALLEL ${UG_ENABLE_PARALLEL})")

finalize_dune_project(GENERATE_CONFIG_H_CMAKE)
