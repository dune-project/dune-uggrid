if(MPI_C_FOUND)
  target_sources_dims(duneuggrid PRIVATE
    cons.cc
    debug.cc
    stat.cc)
endif()

install(FILES
  stat.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/ctrl)
