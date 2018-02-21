set(SOURCES
  disctools.cc
  udm.cc
  formats.cc
)

set(npinclude_HEADERS  udm.h formats.h disctools.h)

ug_add_dim_libs(udm OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
