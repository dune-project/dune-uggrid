install(FILES
  ppif.h
  ppifcontext.hh
  ppiftypes.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ppif)

if(UG_ENABLE_PARALLEL)
  if(MPI_C_FOUND)
    add_library(ppifmpi OBJECT ppif.cc)
    dune_target_link_libraries(ppifmpi ${PROJECT_NAME})
    add_dune_mpi_flags(ppifmpi OBJECT)
    if(DUNE_BUILD_BOTH_LIBS)
      # For shared libraries we need position independent code
      set_property(TARGET ppifmpi PROPERTY POSITION_INDEPENDENT_CODE TRUE)
    endif()
  endif()
endif()
