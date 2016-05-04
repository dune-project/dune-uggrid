add_subdirectory(ugdata)

if(MPI_FOUND)
  set(_parallel_dim_libs parallel ddd analyser basic ctrl ident if join mgr prio xfer dddif)
  set(_parallel_libs ppifmpi parutil)
endif()

set(_dim_libs ug_gm gg np algebra amg field procs udm graphics uggrape uggraph ugui low)


dune_add_library(devS APPEND $<TARGET_OBJECTS:devSo>)
ug_add_dim_libs(ugL SOURCES ../initug.cc OBJECT_DIM_LIBS ${_dim_libs} domL ngin OBJECT_LIBS  devices low)
ug_add_dim_libs(ugS APPEND DUNE SOURCES ../initug.cc
  OBJECT_DIM_LIBS ${_dim_libs} domS ${_parallel_dim_libs}
  OBJECT_LIBS devices low ${_parallel_libs})

if(MPI_FOUND)
  add_dune_mpi_flags(ugS2 ugS3)
endif()


install(TARGETS ugL2 ugL3 DESTINATION ${CMAKE_INSTALL_LIBDIR})
