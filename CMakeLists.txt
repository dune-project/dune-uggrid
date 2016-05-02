# set up project
project("dune-uggrid" C CXX)

#circumvent not building docs
set(BUILD_DOCS 1)

# general stuff
cmake_minimum_required(VERSION 2.8.12)

#find dune-common and set the module path
find_package(dune-common)
list(APPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH}
     "${CMAKE_SOURCE_DIR}/cmake/modules")# make sure our own modules are found

#include the dune macros
include(DuneMacros)

# start a dune project with information from dune.module
dune_project()

find_package(FLEX)
find_package(BISON)

# set defines that are only used internally.
# The rest is in cmake/modules/DuneUggrid.cmake
set(UG_ENABLE_DEBUGGING False CACHE BOOL "Enable UG debugging (default is Off)")
set(UG_ENABLE_2D True CACHE BOOL "Build library for 2d (default is On)")
set(UG_ENABLE_3D True CACHE BOOL "Build library for 3d (default is On)")
set(UG_ENABLE_SYSTEM_HEAP ON CACHE BOOL "If ON/True then we are using the operating system heap instead of the one internal to UG (Default: ON")
set(UG_DDD_MAX_MACROBITS "24" CACHE STRING
  "Set number of bits of an unsigned int used to store the process number,
       the remaining bits are used to store the local entity id")
if(UG_ENABLE_SYSTEM_HEAP)
  add_definitions("-DUG_USE_SYSTEM_HEAP=1")
endif()
if(UG_ENABLE_DEBUGGING)
  add_definitions("-DDebug")
endif()

add_definitions(-DUGLIB -DFOR_DUNE)

#set the include headers
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/low
  ${PROJECT_SOURCE_DIR}/gm ${PROJECT_SOURCE_DIR}/dev ${PROJECT_SOURCE_DIR}/dom
  ${PROJECT_SOURCE_DIR}/np ${PROJECT_SOURCE_DIR}/ui
  ${PROJECT_SOURCE_DIR}/graphics/uggraph ${PROJECT_SOURCE_DIR}/np/algebra
  ${PROJECT_SOURCE_DIR}/np/udm ${PROJECT_SOURCE_DIR}/np/procs
  ${PROJECT_SOURCE_DIR}/parallel ${PROJECT_SOURCE_DIR}/parallel/ddd
  ${PROJECT_SOURCE_DIR}/parallel/ppif ${PROJECT_SOURCE_DIR}/ddd/include
  ${PROJECT_SOURCE_DIR}/parallel/dddif ${PROJECT_SOURCE_DIR}/parallel/util
  ${PROJECT_SOURCE_DIR}/parallel/ddd/include )


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
    #message("_lib=${_lib}")
    set(${var} ${${var}} "\$<TARGET_OBJECTS:${_lib}${dim}>")
  endforeach()
  #message("expanded ${ARGN} with var=${var} dim=${dim} to ${${var}}")
endmacro()
# a macro that creates the libraries for all dimensions.
#
macro(ug_add_dim_libs LIB_NAME)
  #message("create ${LIB_NAME} ${ARGN}")
  cmake_parse_arguments(ADD_LIB  "OBJECT;DUNE" ""
    "SOURCES;SOURCES_2D;SOURCES_3D;OBJECT_LIBS;OBJECT_DIM_LIBS;ADD_LIBS" ${ARGN})
  if(ADD_LIB_OBJECT)
    set(OBJECT "OBJECT")
#    set_property(GLOBAL PROPERTY ${LIB_NAME}_OBJECT_LIBS ${ADD_LIB_OBJECT_LIBS})
#    set(ADD_LIB_OBJECT_LIBS "")
  # else()
  #   foreach(lib ${ADD_LIB_OBJECT_LIBS})
  #     get_property(_saved_libs GLOBAL PROPERTY ${lib}_OBJECT_LIBS)
  #     set(ADD_LIB_OBJECT_LIBS ${ADD_LIB_OBJECT_LIBS} ${saved_libs})
  #   endforeach()
  #   ug_expand_object_libs(OBJECT_LIBS "" ${ADD_LIB_OBJECT_LIBS}
  #     ${_saved_libs})
  endif()

  ug_expand_object_libs(OBJECT_LIBS "" ${ADD_LIB_OBJECT_LIBS})
  foreach(dim  ${UG_ENABLED_DIMENSIONS})
    #message("will expand ${dim} ${ADD_LIB_OBJECT_DIM_LIBS}")
    # if(ADD_LIB_OBJECT)
    #   set_property(GLOBAL PROPERTY ${LIB_NAME}_OBJECT_DIM_LIBS
    #   ${ADD_LIB_OBJECT_DIM_LIBS})
    #   set(ADD_LIB_OBJECT_DIM_LIBS "")
    # else()
    #   foreach(lib ${ADD_LIB_OBJECT_DIM_LIBS})
    #   get_property(_saved_libs GLOBAL PROPERTY ${lib}_OBJECT_DIM_LIBS)
    #   set(ADD_LIB_OBJECT_DIM_LIBS ${ADD_LIB_OBJECT_DIM_LIBS} ${saved_libs})
    #   endforeach()
    # endif()
    ug_expand_object_libs(OBJECT_DIM_LIBS ${dim} ${ADD_LIB_OBJECT_DIM_LIBS})

    if(ADD_LIB_DUNE)
      if(ADD_LIB_ADD_LIBS)
        set(_EXTRA_ARGS "ADD_LIBS" ${ADD_LIB_ADD_LIBS})
      endif()
      dune_add_library("${LIB_NAME}${dim}" ${OBJECT} ${ADD_LIB_SOURCES}
        ${ADD_LIB_SOURCES_${dim}D}
        ${ADD_LIB_UNPARSED_ARGUMENTS} ${OBJECT_DIM_LIBS} ${OBJECT_LIBS}
        ${_EXTRA_ARGS})
      target_compile_definitions("${LIB_NAME}${dim}" PRIVATE "-DUG_DIM_${dim}")
    else()
      add_library("${LIB_NAME}${dim}" ${OBJECT} ${ADD_LIB_SOURCES}
        ${ADD_LIB_SOURCES_${dim}D}
        ${ADD_LIB_UNPARSED_ARGUMENTS} ${OBJECT_DIM_LIBS} ${OBJECT_LIBS})
      if(ADD_LIB_ADD_LIBS)
        dune_target_link_libraries(${LIB_NAME}${dim} ${ADD_LIB_ADD_LIBS})
      endif()
    target_compile_definitions("${LIB_NAME}${dim}" PRIVATE "-DUG_DIM_${dim}")
    endif()
  endforeach()
endmacro()
# !!! maybe make this configurable later
set(DYNAMIC_MEMORY_ALLOCMODEL True)

check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(float.h HAVE_FLOAT_H)
check_include_file(float.h HAVE_LIMITS_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(time.h HAVE_TIME_H)
include(CheckFunctionExists)
# Although the define is checked in low/ugmemory.h
# alloca is never called.
#check_function_exists(alloca HAVE_ALLOCA)
# define it unconditionally
set(HAVE_ALLOCA True)

check_function_exists(bzero HAVE_BZERO)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(memcpy HAVE_MEMCPY)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(realloc HAVE_REALLOX)
check_function_exists(strchr HAVE_STRCHR)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtol HAVE_STRTOL)
# I am not really sure what AC_FUNC_FORK does
# For now just make the checks for defines needed
# for compilation
check_function_exists(vfork HAVE_VFORK)
set(HAVE_WORKING_VFORK "${HAVE_VFORK}")

if(NOT HAVE_ALLOCA_H)
  message(SEND_ERROR "Could not find required header alloca.h")
endif()
if(NOT HAVE_ALLOCA)
  message(SEND_ERROR "Could not find required function alloca")
endif()

if(HAVE_STRING_H OR HAVE_STRINGS_H)
  set(STDC_HEADERS)
endif()

if(HAVE_TIME_H AND HAVE_SYS_TIME_H)
  set(TIME_WITH_SYS_TIME True)
endif()

# do everything in the subdirs (order matters!)
set(SUBDIRS  parallel low dev gm dom graphics np ui lib tools)

set(CMAKE_INSTALL_PKGINCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/ug")

# install pkg-config information
install(FILES libug.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
#install header
install(FILES initug.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
foreach(i ${SUBDIRS})
  if(${i} STREQUAL "test")
    set(opt EXCLUDE_FROM_ALL)
  endif(${i} STREQUAL "test")
  add_subdirectory(${i} ${opt})
  unset(opt)
endforeach(i ${SUBDIRS})

add_subdirectory(cmake/modules)
# set variable names for config.h
set(ARCHNAME ${CMAKE_LIBRARY_ARCHITECTURE})
set(DDD_MAX_PROCBITS_IN_GID ${UG_DDD_MACROBITS})
# finalize the dune project, e.g., generate config.h etc.
finalize_dune_project(GENERATE_CONFIG_H_CMAKE)
