add_subdirectory(ugdata)

if(MPI_FOUND)
  set(_parallel_dim_libs parallel ddd analyser basic ctrl ident if join mgr prio xfer dddif)
  set(_parallel_libs ppifmpi parutil)
endif()

set(_dim_libs ug_gm gg np algebra amg field procs udm graphics uggrape uggraph ugui devices low)

ug_add_dim_libs(ugL SOURCES ../initug.cc OBJECT_DIM_LIBS ${_dim_libs} domL ngin OBJECT_LIBS low)
ug_add_dim_libs(ugS SOURCES ../initug.cc
  OBJECT_DIM_LIBS ${_dim_libs} domS ${_parallel_dim_libs}
  OBJECT_LIBS low ${_parallel_libs})

if(MPI_FOUND)
  add_dune_mpi_flags(ugS2 ugS3)
endif()
