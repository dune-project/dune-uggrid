set(SOURCES  initnp.cc)# initnumerics.h np.h)

ug_add_dim_libs(np OBJECT SOURCES ${SOURCES})
#  OBJECT_DIM_LIBS algebra amg field procs udm)

install(FILES np.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})


set(SOURCES
  sm.cc
)

set(npinclude_HEADERS
  sm.h)

ug_add_dim_libs(algebra OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})




set(SOURCES
  disctools.cc
  udm.cc
  formats.cc
)

set(npinclude_HEADERS  udm.h formats.h disctools.h)

ug_add_dim_libs(udm OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
