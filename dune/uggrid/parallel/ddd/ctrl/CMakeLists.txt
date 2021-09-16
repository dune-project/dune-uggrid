target_sources_dims(dune-uggrid PRIVATE
  cons.cc
  debug.cc
  stat.cc)

install(FILES
  stat.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/ctrl)
