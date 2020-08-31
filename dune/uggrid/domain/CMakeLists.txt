target_sources_dims(duneuggrid PRIVATE
  std_domain.cc
  std_parallel.cc)

install(FILES
  domain.h
  std_domain.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/domain)
