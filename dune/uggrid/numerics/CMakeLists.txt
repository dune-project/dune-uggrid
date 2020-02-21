ug_add_dim_libs(numerics OBJECT SOURCES initnp.cc formats.cc udm.cc)

install(FILES
  formats.h
  udm.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/numerics)
