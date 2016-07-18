# normal headers
set(lowinclude_HEADERS  heaps.h ugenv.h ugstruct.h misc.h debug.h ugtypes.h
  general.h dimension.h tree.h boxtree.h ugtimer.h scan.h
  defaults.h fileopen.h namespace.h)

# special library for the parts of low that need a dimension
set(DIMDEPSOURCES  boxtree.cc)

# dimension-independent lib
add_library(low OBJECT ugenv.cc heaps.cc fifo.cc misc.cc defaults.cc initlow.cc
  fileopen.cc ugstruct.cc debug.cc bio.cc scan.cc ugtimer.cc tree.cc)

if(DUNE_BUILD_BOTH_LIBS)
  # For shared libraries we need position independent code
  set_property(TARGET low PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()
# dimension-dependent libs (2D 3D)
ug_add_dim_libs(low OBJECT SOURCES ${DIMDEPSOURCES} )

install(FILES ${lowinclude_HEADERS} DESTINATION  ${CMAKE_INSTALL_PKGINCLUDEDIR})
