if(UG_ENABLE_PARALLEL)
  set(_parallel_dim_libs ddd analyser ctrl ident if join mgr prio xfer dddif)
  set(_parallel_libs basic ppifmpi)
endif()

set(_dim_libs ug_gm ugui)

foreach(_l ${_parallel_libs})
  list(APPEND _parallel_objs $<TARGET_OBJECTS:${_l}>)
endforeach()

dune_add_library(ugL $<TARGET_OBJECTS:devices> $<TARGET_OBJECTS:low> ${_parallel_objs}
  SOURCES
  ../parallel/ddd/dddcontext.cc
  ../parallel/ppif/ppifcontext.cc
  ADD_LIBS ${DUNE_LIBS}
  )
ug_add_dim_libs(ugS APPEND DUNE SOURCES ../initug.cc
  OBJECT_DIM_LIBS ${_dim_libs} domS ${_parallel_dim_libs}
  ADD_LIBS ugL ${DUNE_LIBS}
  )

if(MPI_C_FOUND)
  add_dune_mpi_flags(ugL)
endif()

install(
  FILES RefRules.data tetra.rls triangle.rls
  DESTINATION ${CMAKE_INSTALL_DATADIR})
