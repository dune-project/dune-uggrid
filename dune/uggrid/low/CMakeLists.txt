add_subdirectory(test)

# dimension-independent lib
add_library(low OBJECT ugenv.cc heaps.cc fifo.cc misc.cc defaults.cc initlow.cc
  fileopen.cc ugstruct.cc debug.cc bio.cc scan.cc ugtimer.cc)

if(DUNE_BUILD_BOTH_LIBS)
  # For shared libraries we need position independent code
  set_property(TARGET low PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()

install(FILES
  debug.h
  defaults.h
  dimension.h
  fileopen.h
  general.h
  heaps.h
  misc.h
  namespace.h
  scan.h
  ugenv.h
  ugstruct.h
  ugtimer.h
  ugtypes.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/low)
