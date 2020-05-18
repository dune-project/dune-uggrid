add_subdirectory(test)

if(MPI_C_FOUND)
  add_library(basic OBJECT io.cc lowcomm.cc notify.cc reduct.cc topo.cc)
  target_link_libraries(basic ${DUNE_LIBS})
  add_dune_mpi_flags(basic OBJECT)
endif()

install(FILES notify.h lowcomm.h oopp.h ooppcc.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/basic)
