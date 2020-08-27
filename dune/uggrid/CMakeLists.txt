# define dimension specific object libraries
foreach(dim ${UG_ENABLED_DIMENSIONS})
  add_library(duneuggrid_${dim}d OBJECT)
  target_include_directories(duneuggrid_${dim}d PRIVATE ddd/include)
  target_compile_definitions(duneuggrid_${dim}d PRIVATE "UG_DIM_${dim}")
  target_compile_definitions(duneuggrid_${dim}d PRIVATE ${UG_COMPILE_DEFINITIONS})
  add_dune_mpi_flags(duneuggrid_${dim}d OBJECT)

  # register dimension-dependent objects in duneuggrid library
  target_sources(duneuggrid PRIVATE $<TARGET_OBJECTS:duneuggrid_${dim}d>)
endforeach()

# macro that adds sources to all dimension-targets `${target}${dim}d`,
# see UG_ENABLED_DIMENSIONS
macro(target_sources_dims target)
  foreach(dim ${UG_ENABLED_DIMENSIONS})
    target_sources(${target}_${dim}d ${ARGN})
  endforeach(dim)
endmacro()


target_sources(duneuggrid PRIVATE
  ugdevices.cc)

target_sources_dims(duneuggrid PRIVATE
  initug.cc
  ugdevices.cc)

install(
  FILES ugdevices.h initug.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid)

add_subdirectory(domain)
add_subdirectory(gm)
add_subdirectory(lib)
add_subdirectory(low)
add_subdirectory(parallel)
