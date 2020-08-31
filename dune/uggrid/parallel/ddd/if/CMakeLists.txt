target_sources_dims(duneuggrid PRIVATE
  ifcheck.cc
  ifcmds.cc
  ifcreate.cc
  ifobjsc.cc
  ifuse.cc)

install(FILES
  if.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/if)
