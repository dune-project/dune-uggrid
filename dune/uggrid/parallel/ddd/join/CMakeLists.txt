target_sources_dims(duneuggrid PRIVATE
  jcmds.cc
  join.cc)

install(FILES
  join.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/join)
