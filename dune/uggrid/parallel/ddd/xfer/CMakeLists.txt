target_sources_dims(duneuggrid PRIVATE
  cmdmsg.cc
  cmds.cc
  cplmsg.cc
  ctrl.cc
  pack.cc
  supp.cc
  unpack.cc
  xfer.cc)

install(FILES
  sll.h xfer.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/xfer)
