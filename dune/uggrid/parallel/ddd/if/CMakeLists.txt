target_sources_dims(duneuggrid PRIVATE
  ifcreate.cc
  ifuse.cc
  ifcmds.cc
  ifcheck.cc
  ifobjsc.cc)

install(FILES
  if.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/if)
