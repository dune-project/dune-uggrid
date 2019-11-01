ug_add_dim_libs(numerics OBJECT SOURCES initnp.cc sm.cc disctools.cc formats.cc udm.cc)

install(FILES
  disctools.h
  formats.h
  np.h
  sm.h
  udm.h
  DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
