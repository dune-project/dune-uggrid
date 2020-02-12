ug_add_dim_libs(numerics OBJECT SOURCES initnp.cc sm.cc formats.cc udm.cc)

install(FILES
  formats.h
  np.h
  sm.h
  udm.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/numerics)
