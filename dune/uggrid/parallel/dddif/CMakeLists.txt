target_sources_dims(duneuggrid PRIVATE
  compat.cc
  debugger.cc
  gridcons.cc
  handler.cc
  identify.cc
  identify.h
  initddd.cc
  lb.cc
  lbrcb.cc
  memmgr.cc
  overlap.cc
  priority.cc
  pgmcheck.cc
  partition.cc
  support.cc
  trans.cc)

install(FILES parallel.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/dddif/)
