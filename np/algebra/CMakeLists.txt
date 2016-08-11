set(SOURCES
  ugblas.cc
  ugiter.cc
  transgrid.cc
  block.cc
  npcheck.cc
  sm.cc
  blasm.cc
)

set(npinclude_HEADERS
  block.h
  sm.h)

ug_add_dim_libs(algebra OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
