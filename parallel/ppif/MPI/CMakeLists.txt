if(MPI_C_FOUND)
  add_library(ppifmpi OBJECT ppif.cc)
  add_dune_mpi_flags(ppifmpi OBJECT)
  if(DUNE_BUILD_BOTH_LIBS)
    # For shared libraries we need position independent code
    set_property(TARGET ppifmpi PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()
endif()
