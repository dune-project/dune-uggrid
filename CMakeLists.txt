# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

cmake_minimum_required(VERSION 3.16)

# set up project
project("dune-uggrid" C CXX)

#circumvent not building docs
set(BUILD_DOCS 1)

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

# deactivate global include-directories
dune_policy(SET DP_DEFAULT_INCLUDE_DIRS NEW)

# do not make suggested dependencies required automatically
dune_policy(SET DP_SUGGESTED_MODULE_DEPENDENCIES_REQUIRED_DOWNSTREAM NEW)

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
  list(APPEND UG_COMPILE_DEFINITIONS "Debug")
  set(UG_EXTRAFLAGS "${UG_EXTRAFLAGS} -DDebug")
endif()

#Always build parallel libs if MPI is found
if(UG_ENABLE_PARALLEL)
  if(NOT MPI_C_FOUND)
    message(SEND_ERROR "A parallel UG was requested but MPI was not found. Either change variable UG_ENABLE_PARALLEL or install MPI." )
  endif()
  list(APPEND UG_COMPILE_DEFINITIONS "ModelP")
  set(UG_EXTRAFLAGS "${UG_EXTRAFLAGS} -DModelP")
endif()

# define target for duneuggrid
dune_add_library(duneuggrid EXPORT_NAME UGGrid)
target_compile_definitions(duneuggrid PUBLIC ${UG_COMPILE_DEFINITIONS})
target_link_libraries(duneuggrid PUBLIC Dune::Common)
add_dune_mpi_flags(duneuggrid)

# set include directories for duneuggrid library
dune_default_include_directories(duneuggrid PUBLIC)

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

# make sure downstream projects must find MPI too
if (UG_ENABLE_PARALLEL)
  string(JOIN "\n" DUNE_CUSTOM_PKG_CONFIG_SECTION
    "find_dependency(MPI)"
  )
endif()

finalize_dune_project()
