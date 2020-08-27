add_subdirectory(test)

target_sources(duneuggrid PRIVATE
  ugenv.cc
  heaps.cc
  fifo.cc
  misc.cc
  initlow.cc
  fileopen.cc
  ugstruct.cc
  debug.cc
  bio.cc
  scan.cc
  ugtimer.cc)

install(FILES
  debug.h
  dimension.h
  fileopen.h
  heaps.h
  misc.h
  namespace.h
  scan.h
  ugenv.h
  ugstruct.h
  ugtimer.h
  ugtypes.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/low)
