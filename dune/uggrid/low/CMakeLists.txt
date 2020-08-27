add_subdirectory(test)

target_sources(duneuggrid PRIVATE
  bio.cc
  debug.cc
  fifo.cc
  fileopen.cc
  heaps.cc
  initlow.cc
  misc.cc
  scan.cc
  ugenv.cc
  ugstruct.cc
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
