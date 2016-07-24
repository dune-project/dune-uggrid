set(SOURCES
  ugblas.cc
  ugiter.cc
  transgrid.cc
  block.cc
  quadrature.cc
  fvgeom.cc
  fegeom.cc
  ff_gen.cc
  ff.cc
  npcheck.cc
  sm.cc
  blasm.cc
  ugeblas.cc)

set(npinclude_HEADERS
  block.h
  fegeom.h
  fvgeom.h
  quadrature.h
  sm.h)

ug_add_dim_libs(algebra OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
