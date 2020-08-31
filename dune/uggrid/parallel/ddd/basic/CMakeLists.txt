add_subdirectory(test)

target_sources(duneuggrid PRIVATE
  io.cc
  lowcomm.cc
  notify.cc
  reduct.cc
  topo.cc)

install(FILES notify.h lowcomm.h oopp.h ooppcc.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/basic)
