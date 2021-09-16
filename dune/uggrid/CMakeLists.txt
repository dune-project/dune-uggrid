set(UG_ENABLED_DIMENSIONS 2 3)

# define dimension specific object libraries
foreach(dim ${UG_ENABLED_DIMENSIONS})
  add_library(dune-uggrid${dim}d OBJECT)
  target_compile_definitions(dune-uggrid${dim}d PRIVATE "UG_DIM_${dim}")
  target_compile_definitions(dune-uggrid${dim}d PRIVATE ${UG_COMPILE_DEFINITIONS})
  target_link_libraries(dune-uggrid${dim}d PRIVATE ${DUNE_LIBS})
  target_include_directories(dune-uggrid${dim}d PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(dune-uggrid${dim}d PRIVATE ${PROJECT_SOURCE_DIR})
  add_dune_mpi_flags(dune-uggrid${dim}d OBJECT)

  # register dimension-dependent objects in dune-uggrid library
  target_sources(dune-uggrid PRIVATE $<TARGET_OBJECTS:dune-uggrid${dim}d>)
endforeach()

# macro that adds sources to all dimension-targets `${target}${dim}d`,
# see UG_ENABLED_DIMENSIONS
macro(target_sources_dims target)
  foreach(dim ${UG_ENABLED_DIMENSIONS})
    target_sources(${target}${dim}d ${ARGN})
  endforeach(dim)
endmacro()


target_sources(dune-uggrid PRIVATE
  ugdevices.cc)

target_sources_dims(dune-uggrid PRIVATE
  initug.cc)

install(
  FILES ugdevices.h initug.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid)

add_subdirectory(domain)
add_subdirectory(gm)
add_subdirectory(lib)
add_subdirectory(low)
add_subdirectory(parallel)
