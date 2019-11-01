ug_add_dim_libs(np OBJECT
  SOURCES
  disctools.cc
  formats.cc)
  initnp.cc
  sm.cc
  udm.cc

install(
  FILES
  udm.h
  formats.h
  disctools.h
  np.h
  sm.h
  DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
