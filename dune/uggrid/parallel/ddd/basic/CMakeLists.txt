add_subdirectory(test)

if(MPI_C_FOUND)
  target_sources(duneuggrid PRIVATE
    io.cc
    lowcomm.cc
    notify.cc
    reduct.cc
    topo.cc)
endif()

install(FILES notify.h lowcomm.h oopp.h ooppcc.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/basic)
