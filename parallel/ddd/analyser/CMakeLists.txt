set(SOURCES  inv.cc)

if(MPI_FOUND)
  ug_add_dim_libs(analyser OBJECT SOURCES ${SOURCES})
endif()
